#include "Battle/Components/CombatEffectsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "Battle/Characters/BtlPlayerCharacter.h"

UCombatEffectsComponent::UCombatEffectsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatEffectsComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCombatEffectsComponent::PlayHitVibration()
{
	if (!HitForceFeedback) return;
	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController()))
		{
			FForceFeedbackParameters Params;
			Params.bLooping = false;
			PC->ClientPlayForceFeedback(HitForceFeedback, Params);
		}
	}
}

void UCombatEffectsComponent::PlayCounterVibration()
{
	if (!CounterForceFeedback) return;
	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController()))
		{
			FForceFeedbackParameters Params;
			Params.bLooping = false;
			PC->ClientPlayForceFeedback(CounterForceFeedback, Params);
		}
	}
}

void UCombatEffectsComponent::StartCameraAssist(AActor* Target)
{
	if (!Target || !GetOwner()) return;
	
	CurrentCameraAssistTarget = Target;
	CameraAssistElapsedTime = 0.0f;

	// 短い間隔で視点補正を更新する
	GetWorld()->GetTimerManager().ClearTimer(CameraAssistTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		CameraAssistTimerHandle,
		this,
		&UCombatEffectsComponent::UpdateCameraAssist,
		0.016f,
		true
	);
}

void UCombatEffectsComponent::UpdateCameraAssist()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!CurrentCameraAssistTarget.IsValid() || !OwnerChar)
	{
		GetWorld()->GetTimerManager().ClearTimer(CameraAssistTimerHandle);
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController());
	if (!PC) return;

	float DeltaTime = GetWorld()->GetDeltaSeconds();
	CameraAssistElapsedTime += DeltaTime;

	// 補正時間が経過したら更新を停止する
	if (CameraAssistElapsedTime >= CameraAssistDuration)
	{
		GetWorld()->GetTimerManager().ClearTimer(CameraAssistTimerHandle);
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FRotator TargetLookAt = UKismetMathLibrary::FindLookAtRotation(CameraLocation, CurrentCameraAssistTarget->GetActorLocation());
	FRotator CurrentControlRot = PC->GetControlRotation();
	TargetLookAt.Pitch = CurrentControlRot.Pitch;
	TargetLookAt.Roll = CurrentControlRot.Roll;

	float YawDelta = FMath::FindDeltaAngleDegrees(CurrentControlRot.Yaw, TargetLookAt.Yaw);
	float ClampedYawDelta = FMath::Clamp(YawDelta, -MaxCameraAssistAngle, MaxCameraAssistAngle);
	
	FRotator ClampedTargetRot = CurrentControlRot;
	ClampedTargetRot.Yaw += ClampedYawDelta;

	float AbsYawDelta = FMath::Abs(YawDelta);
	float BaseInterpSpeed = (AbsYawDelta < 45.0f) ? 4.5f : 2.0f; 
	float SpeedMultiplier = FMath::Clamp(CameraAssistElapsedTime / 0.15f, 0.0f, 1.0f);
	float FinalInterpSpeed = BaseInterpSpeed * SpeedMultiplier;

	FRotator NewControlRot = UKismetMathLibrary::RInterpTo(CurrentControlRot, ClampedTargetRot, DeltaTime, FinalInterpSpeed);
	PC->SetControlRotation(NewControlRot);
}

void UCombatEffectsComponent::PlayReceiveDamageCameraShake()
{
	// カメラシェイククラスが設定されていない場合はスキップ
	if (!ReceiveDamageCameraShake) return;

	// プレイヤーコントローラー経由でカメラシェイクを発火
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->ClientStartCameraShake(ReceiveDamageCameraShake);
	}
}

void UCombatEffectsComponent::PlayCameraShakeAtLocation(TSubclassOf<UCameraShakeBase> ShakeClass, const FVector& Location)
{
	if (!ShakeClass) return;

	UGameplayStatics::PlayWorldCameraShake(
		GetWorld(),
		ShakeClass,
		Location,
		0.0f,
		1000.0f,
		1.0f
	);
}

void UCombatEffectsComponent::PlayCameraShakeDirect(TSubclassOf<UCameraShakeBase> ShakeClass)
{
	if (!ShakeClass) return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	PC->ClientStartCameraShake(ShakeClass);
}

void UCombatEffectsComponent::PlayHitEffect(UNiagaraSystem* HitVFX, const FHitResult& Hit, AActor* TargetActor, EHitReactionType ReactionType)
{
	if (!HitVFX) return;

	FVector SpawnLocation;
	FRotator SpawnRotation;

	// テイクダウンでは対象位置、それ以外では着弾点を基準にする
	if (ReactionType == EHitReactionType::GroundTakedown1 || 
		ReactionType == EHitReactionType::GroundTakedown2)
	{
		AActor* Target = TargetActor ? TargetActor : Hit.GetActor();
		if (!Target) return;

		SpawnLocation = Target->GetActorLocation();
		SpawnRotation = Target->GetActorRotation();
	}
	else
	{
		// 通常攻撃は着弾点と法線から位置・向きを決める
		SpawnLocation = Hit.ImpactPoint;
		SpawnRotation = Hit.ImpactNormal.Rotation();
	}
    
	SpawnHitVFX(HitVFX, SpawnLocation, SpawnRotation);
}

void UCombatEffectsComponent::SpawnHitVFX(UNiagaraSystem* HitVFX, const FVector& Location, const FRotator& Rotation)
{
	if (!IsValid(HitVFX)) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
	   World,
	   HitVFX,
	   Location,
	   Rotation
	);
}

void UCombatEffectsComponent::TriggerAttackCamera(ECombatInputType InputType)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->GetClass()->ImplementsInterface(UAttackCameraInterface::StaticClass())) return;

	if (InputType == ECombatInputType::Normal)
	{
		IAttackCameraInterface::Execute_EnterAttackCamera(OwnerActor, ECombatInputType::Normal);
	}
	else if (InputType == ECombatInputType::Combo)
	{
		IAttackCameraInterface::Execute_EnterAttackCamera(OwnerActor, ECombatInputType::Combo);
	}
}


void UCombatEffectsComponent::PlayAttackSound(ECombatInputType InputType, int32 ComboIndex, UCombatSoundDataAsset* SoundDA)
{
	if (!SoundDA) return;
	
	const FAttackSoundSet* TargetSound = nullptr; 

	if (InputType == ECombatInputType::Normal)
	{
		TargetSound = &SoundDA->Normal;
	}
	else if (InputType == ECombatInputType::Combo)
	{
		if (SoundDA->Combo.IsValidIndex(ComboIndex))
		{
			TargetSound = &SoundDA->Combo[ComboIndex];
		}
	}
	
	if (TargetSound && TargetSound->Whoosh)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, 
			TargetSound->Whoosh, 
			GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector
		);
	}
}

// -----------------------------------------------------------------
// スローモーション (キル時)
// -----------------------------------------------------------------
void UCombatEffectsComponent::TriggerKillSlowMotion()
{
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(SlowMoTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(FinisherTimerHandle);

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), KillTimeDilation);

	// タイマー時間はスローモーション倍率を反映して補正する
	float DilatedDuration = KillSlowDuration * KillTimeDilation;
	GetWorld()->GetTimerManager().SetTimer(
		SlowMoTimerHandle,
		this,
		&UCombatEffectsComponent::ResetTimeDilation,
		DilatedDuration,
		false
	);
}

void UCombatEffectsComponent::PlayForceFeedback(UForceFeedbackEffect* ForceFeedback)
{
	if (!ForceFeedback) return;

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController()))
		{
			FForceFeedbackParameters Params;
			Params.bLooping = false;
			PC->ClientPlayForceFeedback(ForceFeedback, Params);
		}
	}
}

void UCombatEffectsComponent::ResetTimeDilation()
{
	if (GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	}
}

// -----------------------------------------------------------------
// フィニッシャー演出
// -----------------------------------------------------------------
void UCombatEffectsComponent::TriggerFinisherEffect()
{
	if (!GetWorld()) return;

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), FinisherTimeDilation);

	GetWorld()->GetTimerManager().ClearTimer(SlowMoTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(FinisherTimerHandle);

	GetWorld()->GetTimerManager().SetTimer(
		FinisherTimerHandle, 
		this, 
		&UCombatEffectsComponent::ResetFinisherEffect, 
		FinisherSlowDuration, 
		false
	);

	if (ABtlPlayerCharacter* BtlChar = Cast<ABtlPlayerCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* VroidMesh = BtlChar->GetVroidSkeletalMesh())
		{
			if (FinisherOverlayMaterial)
			{
				VroidMesh->SetOverlayMaterial(FinisherOverlayMaterial);
			}
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(FinisherOverlayTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		FinisherOverlayTimerHandle, 
		this, 
		&UCombatEffectsComponent::ResetFinisherOverlay, 
		FinisherOverlayDuration, 
		false
	);
}

void UCombatEffectsComponent::ResetFinisherEffect()
{
	if (GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	}
}

void UCombatEffectsComponent::ResetFinisherOverlay()
{
	if (ABtlPlayerCharacter* BtlChar = Cast<ABtlPlayerCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* VroidMesh = BtlChar->GetVroidSkeletalMesh())
		{
			VroidMesh->SetOverlayMaterial(nullptr);
		}
	}
}