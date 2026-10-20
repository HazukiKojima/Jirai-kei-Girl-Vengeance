#include "Battle/Components/PlayerCombatComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Battle/Characters/BtlEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "MotionWarpingComponent.h"
#include "Battle/Interfaces/DamageableInterface.h"
#include "Battle/Characters/BtlPlayerCharacter.h"
#include "Battle/DataAsset/CombatDataAsset.h"
#include "Battle/Weapon/WoodenCrate.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Battle/Components/CombatEffectsComponent.h"
#include "Battle/Components/EquipmentComponent.h"
#include "Battle/Interfaces/AttackCameraInterface.h"
#include "Components/CapsuleComponent.h"
#include "ContextualAnimTypes.h"
#include "ContextualAnimSceneAsset.h"
#include "ContextualAnimSceneActorComponent.h"
#include "ContextualAnimUtilities.h"
#include "Battle/Frameworks/BtlGameMode.h"

struct FAttackSequence;
struct FDirectionalActionSet;
class UMotionWarpingComponent;

UPlayerCombatComponent::UPlayerCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UPlayerCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateWarpTargetToAttackTarget();
}

void UPlayerCombatComponent::TryExecuteNormalAttack()
{
	// 覚醒・被弾中は攻撃を拒否し、回避中や攻撃中は入力バッファが開いている場合のみ入力を保存する
	if (IsOwnerAwakeningAnimationBlocked())
	{
		UE_LOG(LogTemp, Log, TEXT("Action blocked: Currently in Awakening Animation!"));
		return;
	}

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		UAnimMontage* ActiveMontage = AnimInstance->GetCurrentActiveMontage();
		if (ActiveMontage == LightHitMontage || ActiveMontage == NormalHitMontage)
		{
			return;
		}
	}
	
	if (bIsEvading)
	{
		if (bIsBufferOpen)
		{
			BufferCurrentInputIfOpen(ECombatInputType::Combo);
		}
		return;
	}

	if (!bAttackAvailable)
	{
		UE_LOG(LogTemp, Log, TEXT("bAttackAvailable=false!"));
		BufferCurrentInputIfOpen(ECombatInputType::Combo);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("bAttackAvailable=true!"));
	if (CheckAndExecuteGroundTakedownConditional())
	{
		return;
	}

	ExecuteNormalAttack();
}

void UPlayerCombatComponent::OpenInputBuffer()
{
	bIsBufferOpen = true;
	if (InputBufferComp)
	{
		InputBufferComp->ClearBuffer();
	}
}

void UPlayerCombatComponent::CloseInputBuffer()
{
	bIsBufferOpen = false;

	// カウンターの結果が確定するまでは保存入力を消費しない
	if (bIsWaitingForCounterHit)
	{
		return;
	}
	
	if (InputBufferComp->HasValidBufferedInput())
	{
		SetAttackAvailable(true);

		bIsEvading = false;
		
		FCombatInputCmd Cmd = InputBufferComp->ConsumeInput();
		switch (Cmd.InputType)
		{
		case ECombatInputType::Combo:
		case ECombatInputType::Normal:
			ExecuteNormalAttack(Cmd.Direction);
			break;
		case ECombatInputType::Evade:
			ExecuteEvade(Cmd.Direction);
			break;
		case ECombatInputType::Counter:
			TryCounter();
			break;
		}
	}
	else
	{
		SetAttackAvailable(true);
	}
}

void UPlayerCombatComponent::OnCounterPressed()
{
	if (!CombatDataAsset) return;
	if (IsOwnerAwakeningAnimationBlocked()) return;
	if (bIsEvading)
	{
		if (bIsBufferOpen)
		{
			BufferCurrentInputIfOpen(ECombatInputType::Counter);
		}
		return;
	}

	AActor* CounterTarget = TargetingComp->FindBestCounterTarget(ActiveCounterWindows);

	if (!bAttackAvailable)
	{
		TryBufferOrCancelComboForCounter(CounterTarget);
		return;
	}

	if (!CounterTarget) return;

	ExecuteCounterStandby(CounterTarget);
}

void UPlayerCombatComponent::OnCounterReleased()
{
	// カウンター受付時間はタイマーで管理する
}

void UPlayerCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerChar = Cast<ABtlPlayerCharacter>(GetOwner());
	check(PlayerChar);

	if (!TargetingComp)
	{
		TargetingComp = GetOwner()->FindComponentByClass<UTargetingComponent>();
		if (!TargetingComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("TargetingComp is missing on %s!"), *GetOwner()->GetName());
		}
	}

	if (!CombatEffectsComp)
	{
		CombatEffectsComp = GetOwner()->FindComponentByClass<UCombatEffectsComponent>();
		if (!CombatEffectsComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("CombatEffectsComp is missing on %s!"), *GetOwner()->GetName());
		}
	}

	if (!InputBufferComp)
	{
		InputBufferComp = GetOwner()->FindComponentByClass<UInputBufferComponent>();
		if (!InputBufferComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("InputBufferComp is missing on %s!"), *GetOwner()->GetName());
		}
	}

	if (PlayerChar && PlayerChar->GetMesh())
	{
		if (UAnimInstance* AnimInstance = PlayerChar->GetMesh()->GetAnimInstance())
		{
			AnimInstance->OnMontageBlendingOut.AddDynamic(this, &UPlayerCombatComponent::OnMontageBlendingOut);
			AnimInstance->OnMontageEnded.AddDynamic(this, &UPlayerCombatComponent::OnMontageEnded);
		}
	}

	if (DefaultCombatDataAsset)
	{
		CombatDataAsset = DefaultCombatDataAsset;
	}
}

bool UPlayerCombatComponent::DoAttackTrace_Implementation(FName DamageSourceBone)
{
	AActor* OwnerChar = GetOwner();
	if (!OwnerChar) return false;

	USkeletalMeshComponent* Mesh = OwnerChar->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh) return false;

	AttackTarget = nullptr;
	FVector StartPos = Mesh->GetSocketLocation((DamageSourceBone));
	FVector ForwardVector = OwnerChar->GetActorForwardVector();
	FVector EndPos = StartPos + (ForwardVector * MeleeTraceDistance);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel2));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerChar);

	TArray<FHitResult> OutHits;
	bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(), StartPos, EndPos, MeleeTraceRadius, ObjectTypes, false, ActorsToIgnore, EDrawDebugTrace::None, OutHits, true, FLinearColor::Red, FLinearColor::Green, 2.0f);

	if (bHit)
	{
		ProcessHitResults(OutHits);
	}

	return bHit;
}

void UPlayerCombatComponent::ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType)
{
	if (IsDamageImmuned()) return;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;

	if (bIsWaitingForCounterHit && CurrentAttackData)
	{
		if (HandleCounterHitIfWaiting(AnimInstance)) return;
	}

	if (OwnerChar)
	{
		FVector SpawnLocation = OwnerChar->GetActorLocation();
		FRotator SpawnRotation = OwnerChar->GetActorRotation();

		if (DamageCauser)
		{
			FVector DirToEnemy = DamageCauser->GetActorLocation() - SpawnLocation;
			DirToEnemy.Z = 0.0f;

			if (!DirToEnemy.IsNearlyZero())
			{
				SpawnRotation = DirToEnemy.Rotation();
			}
		}

		if (ReceiveDamageVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				ReceiveDamageVFX,
				SpawnLocation,
				SpawnRotation
			);
		}

		if (ReceiveDamageSound)
		{
			UGameplayStatics::PlaySound2D(this, ReceiveDamageSound);
		}
	}
	
	HandleDamageReaction();
	CombatEffectsComp->PlayReceiveDamageCameraShake();

	Super::ApplyDamage_Implementation(Damage, DamageCauser, HitReactionType);
}

void UPlayerCombatComponent::OpenCounterWindow_Implementation(AActor* Attacker)
{
	Super::OpenCounterWindow_Implementation(Attacker);
	if (!IsValid(Attacker)) return;
	ActiveCounterWindows.Add(Attacker);
	CombatEffectsComp->StartCameraAssist(Attacker);
}

void UPlayerCombatComponent::CloseCounterWindow_Implementation(AActor* Attacker)
{
	Super::CloseCounterWindow_Implementation(Attacker);
	if (!IsValid(Attacker)) return;
	ActiveCounterWindows.Remove(Attacker);;
}

void UPlayerCombatComponent::ProcessHitResults(const TArray<FHitResult>& OutHits)
{
	TArray<AActor*> AlreadyHitActors;
	bool bIsFirstHit = true;

	for (const FHitResult& Hit : OutHits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || AlreadyHitActors.Contains(HitActor)) continue;

		ApplyMeleeDamageToTarget(HitActor, Hit, AlreadyHitActors);
		
		PlayHitEffectsAndSounds(HitActor, Hit, bIsFirstHit);
        
		NotifyAttackHit(CurrentPendingReaction);

		bIsFirstHit = false;
	}
}

void UPlayerCombatComponent::SetTakedownCollisionEnabled(bool bIgnorePawn)
{
	if (!PlayerChar) return;

	UCapsuleComponent* Capsule = PlayerChar->GetCapsuleComponent();
	if (!Capsule) return;

	if (bIgnorePawn)
	{
		Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	}
	else
	{
		Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	}
}

void UPlayerCombatComponent::CleanupInvalidCounters()
{
	for (auto It = ActiveCounterWindows.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void UPlayerCombatComponent::ExecuteCounter_Implementation(AActor* Target)
{
	if (!Target || !CombatDataAsset) return;

	SetAttackAvailable(false);
	AttackTarget = Target;

	ERelativeDirection DeterminedDir = CalculateDirectionToTarget(Target);
	FDirectionalActionSet* ActionSet = CombatDataAsset->DirectionalActions.Find(DeterminedDir);
	if (!ActionSet) return;

	FAttackSequence* Sequence = ActionSet->ActionMap.Find(ECombatInputType::Counter);
	if (!Sequence || Sequence->Attacks.Num() == 0) return;

	UAttackDataAsset* AttackData = Sequence->Attacks[0];
	if (!AttackData) return;

	CurrentAttackData = AttackData;
	CurrentCounterBlockIndex = 0;
	bIsWaitingForCounterHit = true;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;

	if (AnimInstance)
	{
		UpdateWarpTarget(AttackTarget);

		if (AttackData->CounterBlockSequence.Num() > 0)
		{
			UAnimMontage* FirstBlock = AttackData->CounterBlockSequence[0];
			if (FirstBlock)
			{
				FOnMontageBlendingOutStarted BlendOutDelegate;
				BlendOutDelegate.BindUObject(this, &UPlayerCombatComponent::OnCounterBlockMontageBlendingOut);

				AnimInstance->Montage_Play(FirstBlock);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, FirstBlock);

				UE_LOG(LogTemp, Log, TEXT("Counter Started: Warping to target and entering guard loop..."));
			}
		}
	}
	ActiveCounterWindows.Remove(Target);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->GetClass()->ImplementsInterface(UAttackCameraInterface::StaticClass()))
	{
		IAttackCameraInterface::Execute_EnterAttackCamera(OwnerActor, ECombatInputType::Counter);
	}
}

void UPlayerCombatComponent::ExecuteGroundTakedown()
{
	if (!bAttackAvailable || !CombatDataAsset || !PlayerChar) return;

	AActor* BestTarget = nullptr;
	if (TargetingComp)
	{
		BestTarget = TargetingComp->FindBestGroundTakedownTarget(MaxTargetDistance);
	}

	if (!BestTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecuteGroundTakedown: No stunned target found in input direction."));
		return;
	}

	AttackTarget = BestTarget;
	UpdateWarpTarget(AttackTarget);

	const ERelativeDirection Dir = ERelativeDirection::Common;
	if (CombatDataAsset->DirectionalActions.Contains(Dir))
	{
		const FDirectionalActionSet& ActionSet = CombatDataAsset->DirectionalActions[Dir];

		if (ActionSet.ActionMap.Contains(ECombatInputType::GroundTakedown1))
		{
			const FAttackSequence& Sequence = ActionSet.ActionMap[ECombatInputType::GroundTakedown1];

			if (Sequence.Attacks.Num() > 0 && Sequence.Attacks[0]->AttackMontage)
			{
				UAttackDataAsset* AttackData = Sequence.Attacks[0];
				CurrentAttackData = AttackData;

				SetAttackAvailable(false);
				CurrentPendingReaction = EHitReactionType::GroundTakedown1;

				if (UCapsuleComponent* Capsule = PlayerChar->GetCapsuleComponent())
				{
					Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
					UE_LOG(LogTemp, Log, TEXT("GroundTakedown Started: Collision Disabled."));
				}

				if (CombatSoundDA)
				{
					CurrentAttackSound = CombatSoundDA->Special;
					PlayCombatSound(CurrentAttackSound.AttackStartVoice);
				}
				
				PlayerChar->PlayAnimMontage(Sequence.Attacks[0]->AttackMontage);
				IAttackCameraInterface::Execute_EnterAttackCamera(PlayerChar, ECombatInputType::GroundTakedown1);
			}
		}
	}
}

void UPlayerCombatComponent::TryCounter()
{
	if (!bAttackAvailable) return;

	AActor* Target = TargetingComp->FindBestCounterTarget(ActiveCounterWindows);
	if (!Target) return;

	ExecuteCounterStandby(Target);
}

void UPlayerCombatComponent::SetAwakenState(bool bNewAwaken)
{
	bIsAwakened = bNewAwaken;

	if (bIsAwakened)
	{
		if (AwakenCombatDataAsset)
		{
			CombatDataAsset = AwakenCombatDataAsset;
		}
	}
	else
	{
		if (DefaultCombatDataAsset)
		{
			CombatDataAsset = DefaultCombatDataAsset;
		}
		SetAttackAvailable(true);
		UE_LOG(LogTemp, Log, TEXT("Awakening ended. Attack is now available again!"));
	}
}

void UPlayerCombatComponent::ResetCombatStateForAwaken()
{
	// 覚醒開始前の入力と戦闘状態を初期化する
	if (InputBufferComp)
	{
		InputBufferComp->ClearBuffer();
	}
	
	bIsBufferOpen = false;
	
	bIsEvading = false;
	
	CurrentComboIndex = 0;

	SetAttackAvailable(true);
	
	UE_LOG(LogTemp, Log, TEXT("Awaken Started: Combat State & Input Buffers Cleared."));
}

void UPlayerCombatComponent::TryEvade()
{
	if (IsOwnerAwakeningAnimationBlocked())
	{
		UE_LOG(LogTemp, Log, TEXT("Action blocked: Currently in Awakening Animation!"));
		return;
	}

	if (ActiveJustDodgeWindows.Num() > 0)
	{
		TArray<AActor*> DodgeTargets = ActiveJustDodgeWindows;
		ActiveJustDodgeWindows.Empty();

		if (CombatDataAsset)
		{
			FDirectionalActionSet* ActionSet = CombatDataAsset->DirectionalActions.Find(ERelativeDirection::Back);
			if (ActionSet)
			{
				FAttackSequence* Sequence = ActionSet->ActionMap.Find(ECombatInputType::Evade);
				if (Sequence && Sequence->Attacks.Num() > 0 && Sequence->Attacks[0])
				{
					UAttackDataAsset* EvadeData = Sequence->Attacks[0];
					if (EvadeData && EvadeData->HitVFX)
					{
						AActor* OwnerActor = GetOwner();
						FVector SpawnLocation = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
						FRotator SpawnRotation = OwnerActor ? OwnerActor->GetActorRotation() : FRotator::ZeroRotator;

						UNiagaraFunctionLibrary::SpawnSystemAtLocation(
							GetWorld(),
							EvadeData->HitVFX,
							SpawnLocation,
							SpawnRotation
						);
					}
				}
			}
		}
		
		for (AActor* Attacker : DodgeTargets)
		{
			if (IsValid(Attacker))
			{
				UActorComponent* TargetComp = Attacker->FindComponentByInterface(UTargetInterface::StaticClass());
				
				if (TargetComp)
				{
					ITargetInterface::Execute_OnJustDodgeSuccess(TargetComp, GetOwner());
				}
			}
		}
	}

	if (bIsEvading)
	{
		if (bIsBufferOpen)
		{
			BufferCurrentInputIfOpen(ECombatInputType::Evade);
		}
		return;
	}
	
	if (!bAttackAvailable)
	{
		UE_LOG(LogTemp, Log, TEXT("bAttackAvailable=false! Trying to buffer Evade."));
		ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
		FVector InputDir = OwnerChar ? OwnerChar->GetLastMovementInputVector() : FVector::ZeroVector;
		TryBufferOrCancelComboForEvade(InputDir);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("bAttackAvailable=true! Executing Evade immediately."));
	ExecuteEvade();
}

void UPlayerCombatComponent::ExecuteEvade(FVector InputDirection)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || !CombatDataAsset) return;

	ERelativeDirection EvadeDir = ERelativeDirection::Back;
	FVector InputDir = InputDirection;

	if (InputDir.IsNearlyZero() && OwnerChar)
	{
		InputDir = OwnerChar->GetLastMovementInputVector();
	}

	if (!InputDir.IsNearlyZero())
	{
		InputDir.Normalize();
		FVector Forward = OwnerChar->GetActorForwardVector();
		FVector Right = OwnerChar->GetActorRightVector();

		float ForwardDot = FVector::DotProduct(Forward, InputDir);
		float RightDot = FVector::DotProduct(Right, InputDir);

		if (FMath::Abs(ForwardDot) > FMath::Abs(RightDot))
		{
			EvadeDir = (ForwardDot > 0.f) ? ERelativeDirection::Front : ERelativeDirection::Back;
		}
		else
		{
			EvadeDir = (RightDot > 0.f) ? ERelativeDirection::Right : ERelativeDirection::Left;
		}
	}

	FDirectionalActionSet* ActionSet = CombatDataAsset->DirectionalActions.Find(EvadeDir);
	if (!ActionSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecuteEvade: DirectionalActionSet not found!"));
		return;
	}

	FAttackSequence* Sequence = ActionSet->ActionMap.Find(ECombatInputType::Evade);
	if (!Sequence || Sequence->Attacks.Num() == 0 || !Sequence->Attacks[0])
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecuteEvade: Evade sequence not found in DA!"));
		return;
	}

	UAttackDataAsset* AttackData = Sequence->Attacks[0];
	if (!AttackData || !AttackData->AttackMontage) return;

	SetAttackAvailable(false);
	bIsEvading = true;
	CurrentAttackData = AttackData;

	if (CombatSoundDA)
	{if (CombatFeedbackDA)
	{
		PlayCombatFeedback(CombatFeedbackDA->Evade.Movement);
	}
		PlayCombatSound(CombatSoundDA->Evade.Voice);
		PlayCombatSound(CombatSoundDA->Evade.MovementSE);
		PlayCombatSound(CombatSoundDA->Evade.JustEvadeSE);
	}
	
	if (CombatFeedbackDA)
	{
		PlayCombatFeedback(CombatFeedbackDA->Evade.Movement);
	}
	
	OwnerChar->PlayAnimMontage(AttackData->AttackMontage);
	UE_LOG(LogTemp, Log, TEXT("Evade Started via DA! Direction: %d"), (int32)EvadeDir);
}

void UPlayerCombatComponent::ExecuteSpecialMove()
{
	if (!SpecialMoveSceneAsset) return;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || !TargetingComp) return;

	AActor* Target = TargetingComp->FindBestTarget({ OwnerChar }, 3000.f, nullptr, 0);
	if (!Target) return;

	UContextualAnimSceneActorComponent* SceneComp = OwnerChar->FindComponentByClass<UContextualAnimSceneActorComponent>();
	if (!SceneComp) return;

	TMap<FName, FContextualAnimSceneBindingContext> RoleToContextMap;
	RoleToContextMap.Add(TEXT("Attacker"), FContextualAnimSceneBindingContext(OwnerChar));
	RoleToContextMap.Add(TEXT("Defender"), FContextualAnimSceneBindingContext(Target));

	FContextualAnimSceneBindings Bindings;
	if (!UContextualAnimUtilities::BP_CreateContextualAnimSceneBindings(SpecialMoveSceneAsset, RoleToContextMap, Bindings))
	{
		return;
	}

	if (SceneComp->StartContextualAnimScene(Bindings))
	{
		SetAttackAvailable(false);
	}
}

void UPlayerCombatComponent::SetEvadeState(bool bIsInvincible)
{
	bIsEvading = bIsInvincible;
}

int32 UPlayerCombatComponent::GetComboIndex() const
{
	return CurrentComboIndex;
}

void UPlayerCombatComponent::SetGlobalComboCount(int32 NewComboCount)
{
	GlobalComboCount = NewComboCount;
}

void UPlayerCombatComponent::ExecuteNormalAttack(FVector InputDirection)
{
	UE_LOG(LogTemp, Log, TEXT("ExecuteNormalAttack! - START"));

	if (!CombatDataAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("ExecuteNormalAttack: CombatDataAsset is NULL!"));
		return;
	}

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || !PlayerChar) return;

	FVector InputDir = InputDirection;
	if (InputDir.IsNearlyZero())
	{
		InputDir = OwnerChar->GetLastMovementInputVector();
	}

	AActor* CurrentTarget = DetermineAttackTarget();
	if (!CurrentTarget)
	{
		SetAttackAvailable(true);
		if (InputBufferComp)
		{
			InputBufferComp->ClearBuffer();
		}
		UE_LOG(LogTemp, Warning, TEXT("ExecuteNormalAttack: No target found. State reset to available."));
		return;
	}

	ECombatInputType InputToUse = DetermineCombatInputType(CurrentTarget);
	CurrentExecutingInputType = InputToUse;

	int32 Index = 0;
	UAttackDataAsset* AttackData = GetAttackDataForInput(InputToUse, Index);

	PlayAttackSequence(AttackData, InputToUse, Index, CurrentTarget);
}

bool UPlayerCombatComponent::CheckAndExecuteGroundTakedownConditional()
{
	if (!CombatDataAsset || !PlayerChar) return false;

	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABtlEnemyCharacter::StaticClass(), EnemyActors);

	if (EnemyActors.Num() == 0) return false;

	int32 StunnedCount = 0;
	int32 TotalActiveEnemies = 0;

	for (AActor* Actor : EnemyActors)
	{
		ABtlEnemyCharacter* Enemy = Cast<ABtlEnemyCharacter>(Actor);
		if (!Enemy) continue;

		TotalActiveEnemies++;
		if (Enemy->IsDown())
		{
			StunnedCount++;
		}
	}

	if (TotalActiveEnemies > 0 && StunnedCount == TotalActiveEnemies)
	{
		UE_LOG(LogTemp, Log, TEXT("All enemies are stunned! Redirecting input to GroundTakedown."));
		ExecuteGroundTakedown();
		return true;
	}

	return false;
}

void UPlayerCombatComponent::OnDeath()
{
	Super::OnDeath();

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || DeathMontage == nullptr) return;

	OwnerChar->PlayAnimMontage(DeathMontage);
}

void UPlayerCombatComponent::OnCounterBlockMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted || !CurrentAttackData) return;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	const TArray<UAnimMontage*>& BlockSequence = CurrentAttackData->CounterBlockSequence;

	// 開始ガード後は待機ループへ移行し、ループがなければ反撃を開始する
	if (CurrentCounterBlockIndex == 0)
	{
		if (BlockSequence.IsValidIndex(1) && BlockSequence[1])
		{
			CurrentCounterBlockIndex = 1;
			UAnimMontage* LoopBlock = BlockSequence[CurrentCounterBlockIndex];
			PlayCounterBlockMontageWithDelegate(AnimInstance, LoopBlock);
			AnimInstance->Montage_SetNextSection(FName("Default"), FName("Default"), LoopBlock);
			bIsWaitingForCounterHit = true;
			return;
		}
		else
		{
			bIsWaitingForCounterHit = false;
			if (CurrentAttackData && CurrentAttackData->AttackMontage)
			{
				UpdateWarpTarget(AttackTarget);
				AnimInstance->Montage_Play(CurrentAttackData->AttackMontage);
				CurrentPendingReaction = CurrentAttackData->HitReaction;
				CurrentDamageMultiplier = CurrentAttackData->DamageMultiplier;
				CurrentCounterBlockIndex = 3;
			}
			else
			{
				SetAttackAvailable(true);
			}
			return;
		}
	}
	else if (CurrentCounterBlockIndex == 2)
	{
		// カウンター成立演出の後に反撃モンタージュへ進む
		if (CurrentAttackData->AttackMontage)
		{
			UpdateWarpTarget(AttackTarget);
			AnimInstance->Montage_Play(CurrentAttackData->AttackMontage);
			CurrentPendingReaction = CurrentAttackData->HitReaction;
			CurrentDamageMultiplier = CurrentAttackData->DamageMultiplier;
			CurrentCounterBlockIndex = 3;
		}
		else
		{
			SetAttackAvailable(true);
		}
	}
	else if (CurrentCounterBlockIndex == 3)
	{
		SetAttackAvailable(true);
		CurrentComboIndex = 0;
	}
}

void UPlayerCombatComponent::OpenJustDodgeWindow_Implementation(AActor* Attacker)
{
	if (!IsValid(Attacker)) return;
	ActiveJustDodgeWindows.Add(Attacker);
}

void UPlayerCombatComponent::CloseJustDodgeWindow_Implementation(AActor* Attacker)
{
	if (!IsValid(Attacker)) return;
	ActiveJustDodgeWindows.Remove(Attacker);
}

void UPlayerCombatComponent::PlayEffect(const FHitResult& Hit)
{
	if (CurrentAttackData && CurrentAttackData->HitVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), CurrentAttackData->HitVFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
}

void UPlayerCombatComponent::PlayEffect(AActor* TargetActor)
{
	if (!TargetActor) return;
	if (CurrentAttackData && CurrentAttackData->HitVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), CurrentAttackData->HitVFX, TargetActor->GetActorLocation(), TargetActor->GetActorRotation());
	}
}

void UPlayerCombatComponent::UpdateWarpTarget(AActor* Target)
{
	if (!Target) return;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	UMotionWarpingComponent* MotionWarpingComp = OwnerChar->FindComponentByClass<UMotionWarpingComponent>();
	if (MotionWarpingComp)
	{
		FVector TargetLocation = Target->GetActorLocation();
		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
			OwnerChar->GetActorLocation(), TargetLocation);

		MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(
			TEXT("LongRangeAttack"), TargetLocation, LookAtRotation);
	}
}

void UPlayerCombatComponent::NotifyAttackHit(EHitReactionType HitReactionType)
{
	if (ABtlGameMode* GM = GetWorld()->GetAuthGameMode<ABtlGameMode>())
	{
	}
}

void UPlayerCombatComponent::PlayCombatSound(USoundBase* SoundToPlay)
{
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySound2D(this, SoundToPlay);
	}
}

void UPlayerCombatComponent::PlayCombatFeedback(UForceFeedbackEffect* FeedbackToPlay)
{
	if (FeedbackToPlay && CombatEffectsComp)
	{
		CombatEffectsComp->PlayForceFeedback(FeedbackToPlay);
	}
}

UAnimMontage* UPlayerCombatComponent::SelectAnimationByDistance()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return nullptr;

	FVector InputDir = OwnerChar->GetLastMovementInputVector();
	if (InputDir.IsNearlyZero()) return nullptr;
	InputDir.Normalize();

	FVector Forward = OwnerChar->GetActorForwardVector();
	FVector Right = OwnerChar->GetActorRightVector();

	constexpr float DirectionThreshold = 0.7f;
	float ForwardDot = FVector::DotProduct(Forward, InputDir);
	float RightDot = FVector::DotProduct(Right, InputDir);

	TArray<UAnimMontage*>* PunchArray = nullptr;
	TArray<UAnimMontage*>* KickArray = nullptr;

	if (ForwardDot > DirectionThreshold)
	{
		PunchArray = &AttackPunchFrontMontage;
		KickArray = &AttackKickFrontMontage;
	}
	else if (ForwardDot < -DirectionThreshold)
	{
		PunchArray = &AttackPunchBackMontage;
		KickArray = &AttackKickBackMontage;
	}
	else if (RightDot > DirectionThreshold)
	{
		PunchArray = &AttackPunchRightMontage;
		KickArray = &AttackKickRightMontage;
	}
	else if (RightDot < -DirectionThreshold)
	{
		PunchArray = &AttackPunchLeftMontage;
		KickArray = &AttackKickLeftMontage;
	}
	else
	{
		PunchArray = &AttackPunchFrontMontage;
		KickArray = &AttackKickFrontMontage;
	}

	bool bUsePunch = FMath::RandBool();
	TArray<UAnimMontage*>* SelectedArray = bUsePunch ? PunchArray : KickArray;

	if (!SelectedArray || SelectedArray->Num() == 0) return nullptr;

	int32 Index = FMath::RandRange(0, SelectedArray->Num() - 1);
	UAnimMontage* SelectedMontage = (*SelectedArray)[Index];

	return SelectedMontage;
}

UAnimMontage* UPlayerCombatComponent::SelectCounterMontageByDir(ERelativeDirection Dir)
{
	switch (Dir)
	{
	case ERelativeDirection::Front: return CounterMontageFront;
	case ERelativeDirection::Back: return CounterMontageBack;
	case ERelativeDirection::Left: return CounterMontageLeft;
	case ERelativeDirection::Right: return CounterMontageRight;
	default: return CounterMontageFront;
	}
}

UAnimMontage* UPlayerCombatComponent::SelectCounterMontage(AActor* Target)
{
	if (!Target || !CombatDataAsset) return nullptr;

	ERelativeDirection Dir = CalculateDirectionToTarget(Target);
	FDirectionalActionSet* ActionSet = CombatDataAsset->DirectionalActions.Find(Dir);
	if (!ActionSet) return nullptr;

	FAttackSequence* Sequence = ActionSet->ActionMap.Find(ECombatInputType::Counter);
	if (Sequence && Sequence->Attacks.Num() > 0 && Sequence->Attacks[0])
	{
		return Sequence->Attacks[0]->AttackMontage;
	}
	return nullptr;
}

void UPlayerCombatComponent::OnMontageStarted(UAnimMontage* Montage)
{
}

void UPlayerCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!PlayerChar || !Montage) return;
	PlayerChar->ExitAttackCamera();
}

void UPlayerCombatComponent::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (!PlayerChar || !Montage) return;

	UAnimInstance* AnimInstance = PlayerChar->GetMesh() ? PlayerChar->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	// 被弾モンタージュのブレンドアウト時に攻撃可否と無敵時間を処理する
	if (Montage == LightHitMontage || Montage == NormalHitMontage)
	{
		// 中断時は攻撃を再開可能にしない
		if (!bInterrupted)
		{
			SetAttackAvailable(true);
		}

		if (HitInvincibilityDuration > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(
				HitInvincibilityTimerHandle,
				this,
				&UPlayerCombatComponent::ClearHitInvincibility,
				HitInvincibilityDuration,
				false
			);
		}
		else
		{
			ClearHitInvincibility();
		}
		return;
	}
	
	if (bIsEvading)
	{
		if (CurrentAttackData && Montage == CurrentAttackData->AttackMontage)
		{
			bIsEvading = false;
			if (!bInterrupted)
			{
				SetAttackAvailable(true);
			}
		}
		return;
	}

	if (CurrentAttackData && Montage == CurrentAttackData->AttackMontage)
	{
		if (!bInterrupted)
		{
			SetAttackAvailable(true);
			CurrentComboIndex = 0; 
			bIsBufferOpen = false;
		}
	}

	// 1段目のテイクダウン後に2段目へ遷移するか確認する
	UAnimMontage* Takedown1Montage = nullptr;
	if (CombatDataAsset)
	{
		FDirectionalActionSet* ActionSet = CombatDataAsset->DirectionalActions.Find(ERelativeDirection::Common);
		if (ActionSet)
		{
			FAttackSequence* Seq1 = ActionSet->ActionMap.Find(ECombatInputType::GroundTakedown1);
			if (Seq1 && Seq1->Attacks.Num() > 0)
			{
				Takedown1Montage = Seq1->Attacks[0]->AttackMontage;
			}
		}
	}

	if (Montage == Takedown1Montage && CurrentPendingReaction == EHitReactionType::GroundTakedown1)
	{
		if (CombatDataAsset)
		{
			FDirectionalActionSet* ActionSet = CombatDataAsset->DirectionalActions.Find(ERelativeDirection::Common);
			if (ActionSet)
			{
				FAttackSequence* Seq2 = ActionSet->ActionMap.Find(ECombatInputType::GroundTakedown2);
				if (Seq2 && Seq2->Attacks.Num() > 0 && Seq2->Attacks[0]->AttackMontage)
				{
					UAnimMontage* NextMontage = Seq2->Attacks[0]->AttackMontage;
					SetAttackAvailable(false);
					CurrentAttackData = Seq2->Attacks[0];
					AnimInstance->Montage_Play(NextMontage);
					CurrentPendingReaction = EHitReactionType::GroundTakedown2;
					return;
				}
			}
		}
	}
}

void UPlayerCombatComponent::SetAttackAvailable(bool bEnable)
{
	bAttackAvailable = bEnable;
}

void UPlayerCombatComponent::OnKilledEnemy_Implementation()
{
	Super::OnKilledEnemy_Implementation();
	CombatEffectsComp->TriggerKillSlowMotion();
}

void UPlayerCombatComponent::OnCounterTimeout()
{
	if (CurrentCounterBlockIndex == 2 || !CurrentAttackData) return;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	bIsWaitingForCounterHit = false;
	CurrentCounterBlockIndex = 3;

	const TArray<UAnimMontage*>& BlockSequence = CurrentAttackData->CounterBlockSequence;
	if (BlockSequence.IsValidIndex(3) && BlockSequence[3])
	{
		UAnimMontage* EndBlock = BlockSequence[3];

		FOnMontageBlendingOutStarted BlendOutDelegate;
		BlendOutDelegate.BindUObject(this, &UPlayerCombatComponent::OnCounterBlockMontageBlendingOut);

		AnimInstance->Montage_Play(EndBlock);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, EndBlock);
	}
	else
	{
		SetAttackAvailable(true);
	}
}


bool UPlayerCombatComponent::IsOwnerAwakeningAnimationBlocked() const
{
	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (UEquipmentComponent* EquipComp = OwnerChar->FindComponentByClass<UEquipmentComponent>())
		{
			return EquipComp->bIsAwakening;
		}
	}
	return false;
}

bool UPlayerCombatComponent::BufferCurrentInputIfOpen(ECombatInputType InputType) const
{
	// カウンターの攻撃待ち中は入力を保存しない
	if (bIsWaitingForCounterHit)
	{
		return false;
	}

	if (bIsBufferOpen && InputBufferComp)
	{
		FVector InputDir = Cast<ACharacter>(GetOwner())->GetLastMovementInputVector();
		InputBufferComp->BufferInput(InputType, InputDir);
		UE_LOG(LogTemp, Log, TEXT("Input Buffered via Component! Type: %d"), (int32)InputType);
		return true;
	}
	return false;
}

bool UPlayerCombatComponent::IsDamageImmuned() const
{
	if (IsOwnerAwakeningAnimationBlocked()) return true;
	if (bIsHitInvincible) return true;
	if (bIsEvading) return true;

	if (CurrentExecutingInputType == ECombatInputType::Counter)
	{
		// 反撃待ちの間は攻撃を受けない
		if (bIsWaitingForCounterHit) return true;

		// カウンター用モンタージュの再生中は攻撃を受けない
		if (CurrentAttackData)
		{
			ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
			UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
			if (AnimInstance)
			{
				UAnimMontage* ActiveMontage = AnimInstance->GetCurrentActiveMontage();
				if (ActiveMontage)
				{
					if (CurrentAttackData->CounterBlockSequence.Contains(ActiveMontage) ||
						CurrentAttackData->AttackMontage == ActiveMontage)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

void UPlayerCombatComponent::ClearHitInvincibility()
{
	bIsHitInvincible = false;
	UE_LOG(LogTemp, Log, TEXT("Hit Invincibility Ended (After 1.5s Delay)."));
}

void UPlayerCombatComponent::UpdateWarpTargetToAttackTarget()
{
	if (AttackTarget)
	{
		ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
		UMotionWarpingComponent* MotionWarpingComp = OwnerChar->FindComponentByClass<UMotionWarpingComponent>();
		if (MotionWarpingComp)
		{
			FVector TargetLocation = AttackTarget->GetActorLocation();
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(OwnerChar->GetActorLocation(), TargetLocation);
			MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("LongRangeAttack"), TargetLocation, LookAtRotation);
		}
	}
}

AActor* UPlayerCombatComponent::DetermineAttackTarget()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return nullptr;

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(OwnerChar);
	float SearchRadius = 3000.f;

	if (TargetingComp)
	{
		return TargetingComp->FindBestTarget(
			IgnoredActors,
			SearchRadius,
			LastHitTarget.Get(),
			CurrentComboIndex
		);
	}
	return nullptr;
}

ECombatInputType UPlayerCombatComponent::DetermineCombatInputType(AActor* Target)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	float Distance = FVector::Dist(OwnerChar->GetActorLocation(), Target->GetActorLocation());

	if (Distance > CloseRangeThreshold)
	{
		CurrentComboDirection = CalculateDirectionToTarget(Target);
		return ECombatInputType::Normal;
	}
	else
	{
		// 対象が変わったかコンボ未開始なら、段数を戻して系列を選び直す
		if (LastHitTarget.Get() != Target || CurrentComboIndex <= 0)
		{
			CurrentComboIndex = 0;
			CurrentComboDirection = CalculateDirectionToTarget(Target);
			if (CombatDataAsset->ComboGroups.Num() > 0)
			{
				CurrentComboGroupIndex = FMath::RandRange(0, CombatDataAsset->ComboGroups.Num() - 1);
				UE_LOG(LogTemp, Log, TEXT("Selected Random Combo Group Index: %d"), CurrentComboGroupIndex);
			}
		}
		return ECombatInputType::Combo;
	}
}

UAttackDataAsset* UPlayerCombatComponent::GetAttackDataForInput(ECombatInputType InputType, int32& OutIndex)
{
	UAttackDataAsset* AttackData = nullptr;
	OutIndex = 0;

	if (InputType == ECombatInputType::Combo)
	{
		if (!CombatDataAsset->ComboGroups.IsValidIndex(CurrentComboGroupIndex)) return nullptr;
		const FComboGroup& SelectedGroup = CombatDataAsset->ComboGroups[CurrentComboGroupIndex];
		if (SelectedGroup.Attacks.Num() == 0) return nullptr;

		// コンボ段数をグループ内の攻撃数で循環させる
		OutIndex = CurrentComboIndex % SelectedGroup.Attacks.Num();
		AttackData = SelectedGroup.Attacks[OutIndex];
	}
	else
	{
		FDirectionalActionSet* ActionSet = CombatDataAsset->DirectionalActions.Find(CurrentComboDirection);
		if (!ActionSet) return nullptr;

		FAttackSequence* Sequence = ActionSet->ActionMap.Find(InputType);
		if (!Sequence || Sequence->Attacks.Num() == 0) return nullptr;

		OutIndex = FMath::RandRange(0, Sequence->Attacks.Num() - 1);
		AttackData = Sequence->Attacks[OutIndex];
	}

	return AttackData;
}

void UPlayerCombatComponent::PlayAttackSequence(UAttackDataAsset* AttackData, ECombatInputType InputType, int32 Index, AActor* Target)
{
	if (!AttackData || !AttackData->AttackMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("ExecuteNormalAttack: AttackData or AttackMontage is NULL!"));
		return;
	}

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	UE_LOG(LogTemp, Log, TEXT("ExecuteNormalAttack: Playing Montage -> %s"), *AttackData->AttackMontage->GetName());

	CurrentAttackData = AttackData;
	AttackTarget = Target;

	UpdateWarpTarget(AttackTarget);

	SetAttackAvailable(false);
	OwnerChar->PlayAnimMontage(AttackData->AttackMontage);

	if (CombatSoundDA)
	{
		if (InputType == ECombatInputType::Normal)
		{
			CurrentAttackSound = CombatSoundDA->Normal;
		}
		else if (InputType == ECombatInputType::Combo)
		{
			if (CombatSoundDA->Combo.IsValidIndex(Index))
			{
				CurrentAttackSound = CombatSoundDA->Combo[Index];
			}
		}

		PlayCombatSound(CurrentAttackSound.AttackVoice);
	}

	if (CombatFeedbackDA)
	{
		if (InputType == ECombatInputType::Normal)
		{
			CurrentAttackFeedback = CombatFeedbackDA->Normal;
		}
		else if (InputType == ECombatInputType::Combo)
		{
			if (CombatFeedbackDA->Combo.IsValidIndex(Index))
			{
				CurrentAttackFeedback = CombatFeedbackDA->Combo[Index];
			}
		}
	}
	
	CombatEffectsComp->PlayAttackSound(InputType, Index, CombatSoundDA);

	int32 MaxComboInGroup = (InputType == ECombatInputType::Combo) ? CombatDataAsset->ComboGroups[CurrentComboGroupIndex].Attacks.Num() : 0;
	if (InputType == ECombatInputType::Combo && Index == MaxComboInGroup - 1)
	{
		CombatEffectsComp->TriggerFinisherEffect();
	}

	CombatEffectsComp->TriggerAttackCamera(InputType);

	LastHitTarget = Target;
	CurrentPendingReaction = AttackData->HitReaction;
	CurrentDamageMultiplier = AttackData->DamageMultiplier;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->GetClass()->ImplementsInterface(UAttackCameraInterface::StaticClass())) return;

	if (InputType == ECombatInputType::Normal)
	{
		IAttackCameraInterface::Execute_EnterAttackCamera(OwnerActor, ECombatInputType::Normal);
	}
	else if (InputType == ECombatInputType::Combo)
	{
		IAttackCameraInterface::Execute_EnterAttackCamera(OwnerActor, ECombatInputType::Combo);
		CurrentComboIndex++;
	}
}

void UPlayerCombatComponent::CancelComboForCounter(UAnimInstance* AnimInstance)
{
	if (AnimInstance) AnimInstance->Montage_Stop(0.1f, AnimInstance->GetCurrentActiveMontage());
	if (InputBufferComp) InputBufferComp->ClearBuffer();
	bIsBufferOpen = false;
	CurrentComboIndex = 0;
	SetAttackAvailable(true);
}

void UPlayerCombatComponent::PlayCounterBlockMontageWithDelegate(UAnimInstance* AnimInstance, UAnimMontage* Montage)
{
	if (!AnimInstance || !Montage) return;
	FOnMontageBlendingOutStarted BlendOutDelegate;
	BlendOutDelegate.BindUObject(this, &UPlayerCombatComponent::OnCounterBlockMontageBlendingOut);
	AnimInstance->Montage_Play(Montage);
	AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, Montage);
}

bool UPlayerCombatComponent::HandleCounterHitIfWaiting(UAnimInstance* AnimInstance)
{
	if (!AnimInstance || !CurrentAttackData) return false;

	// カウンター成立後は受付タイマーを止め、成功時の演出へ切り替える
	GetWorld()->GetTimerManager().ClearTimer(CounterTimerHandle);
	bIsWaitingForCounterHit = false;
	CurrentCounterBlockIndex = 2;

	const TArray<UAnimMontage*>& BlockSequence = CurrentAttackData->CounterBlockSequence;
	if (BlockSequence.IsValidIndex(2) && BlockSequence[2])
	{
		UAnimMontage* CounterAttackBlock = BlockSequence[2];
		PlayCounterBlockMontageWithDelegate(AnimInstance, CounterAttackBlock);
		return true;
	}
	return false;
}

void UPlayerCombatComponent::TryBufferOrCancelComboForCounter(AActor* CounterTarget)
{
	if (CurrentExecutingInputType == ECombatInputType::Combo && CounterTarget)
	{
		ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
		UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
		CancelComboForCounter(AnimInstance);
		ExecuteCounterStandby(CounterTarget);
	}
	else
	{
		BufferCurrentInputIfOpen(ECombatInputType::Counter);
	}
}

void UPlayerCombatComponent::ExecuteCounterStandby(AActor* CounterTarget)
{
	CurrentExecutingInputType = ECombatInputType::Counter;
	SetAttackAvailable(false);
	AttackTarget = CounterTarget;

	if (CombatEffectsComp)
	{
		CombatEffectsComp->PlayCounterVibration();
	}
	
	GlobalComboCount++;
	OnCombatInputExecuted.Broadcast(ECombatInputType::Counter, GlobalComboCount);
	
	ERelativeDirection DeterminedDir = CalculateDirectionToTarget(CounterTarget);
	FDirectionalActionSet* ActionSet = CombatDataAsset->DirectionalActions.Find(DeterminedDir);
	if (!ActionSet) return;

	FAttackSequence* Sequence = ActionSet->ActionMap.Find(ECombatInputType::Counter);
	if (!Sequence || Sequence->Attacks.Num() == 0) return;

	UAttackDataAsset* AttackData = Sequence->Attacks[0];
	if (!AttackData) return;

	CurrentAttackData = AttackData;
	CurrentCounterBlockIndex = 0;

	if (CombatSoundDA)
	{
		CurrentAttackSound = CombatSoundDA->Counter;
		PlayCombatSound(CurrentAttackSound.AttackStartVoice);
	}
	
	if (CombatFeedbackDA)
	{
		CurrentAttackFeedback = CombatFeedbackDA->Counter;
		PlayCombatFeedback(CurrentAttackFeedback.Swing);
	}
	
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;

	if (AnimInstance)
	{
		UpdateWarpTarget(AttackTarget);
		const TArray<UAnimMontage*>& BlockSequence = CurrentAttackData->CounterBlockSequence;
		if (BlockSequence.IsValidIndex(0) && BlockSequence[0])
		{
			UAnimMontage* StartBlock = BlockSequence[0];
			PlayCounterBlockMontageWithDelegate(AnimInstance, StartBlock);
			bIsWaitingForCounterHit = true;
			UE_LOG(LogTemp, Log, TEXT("Warping to Target and entering infinite guard loop until hit!"));
		}
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->GetClass()->ImplementsInterface(UAttackCameraInterface::StaticClass()))
	{
		IAttackCameraInterface::Execute_EnterAttackCamera(OwnerActor, ECombatInputType::Counter);
	}
}

void UPlayerCombatComponent::TryBufferOrCancelComboForEvade(FVector InputDirection)
{
	// 通常攻撃またはコンボ中は、攻撃を中断して回避する
	if (CurrentExecutingInputType == ECombatInputType::Combo || CurrentExecutingInputType == ECombatInputType::Normal)
	{
		ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
		UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
        
		// 被弾モンタージュ中は攻撃を中断できない
		if (AnimInstance)
		{
			UAnimMontage* ActiveMontage = AnimInstance->GetCurrentActiveMontage();
			if (ActiveMontage == LightHitMontage || ActiveMontage == NormalHitMontage)
			{
				return;
			}
		}
		if (InputBufferComp)
		{
			InputBufferComp->ClearBuffer();
		}
		bIsBufferOpen = false;
		CurrentComboIndex = 0;
		SetAttackAvailable(true);

		ExecuteEvade(InputDirection);
	}
	else
	{
		// 攻撃を中断できない場合は回避入力をバッファに登録する
		BufferCurrentInputIfOpen(ECombatInputType::Evade);
	}
}

void UPlayerCombatComponent::HandleDamageReaction()
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = OwnerChar ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;

	bIsBufferOpen = false;
	if (InputBufferComp)
	{
		InputBufferComp->ClearBuffer();
	}
	CurrentComboIndex = 0;
	
	SetGlobalComboCount(0);
	OnComboReset.Broadcast();
	
	GetWorld()->GetTimerManager().ClearTimer(HitInvincibilityTimerHandle);
	bIsHitInvincible = true;
	
	if (bAttackAvailable)
	{
		if (OwnerChar && LightHitMontage)
		{
			SetAttackAvailable(false);
			OwnerChar->PlayAnimMontage(LightHitMontage);
		}
	}
	else
	{
		bool bIsTakedown = (CurrentPendingReaction == EHitReactionType::GroundTakedown1 ||
			CurrentPendingReaction == EHitReactionType::GroundTakedown2);

		if (OwnerChar && AnimInstance && !bIsTakedown)
		{
			AnimInstance->Montage_Stop(0.2f, AnimInstance->GetCurrentActiveMontage());

			CurrentComboIndex = 0;
			SetAttackAvailable(false);

			if (NormalHitMontage)
			{
				OwnerChar->PlayAnimMontage(NormalHitMontage);
			}
			else if (LightHitMontage)
			{
				OwnerChar->PlayAnimMontage(LightHitMontage);
			}

			UE_LOG(LogTemp, Log, TEXT("Attack Interrupted by Enemy Hit! Attack Locked."));
		}
	}
}

void UPlayerCombatComponent::ApplyMeleeDamageToTarget(AActor* HitActor, const FHitResult& Hit, TArray<AActor*>& InOutAlreadyHitActors)
{
	float FinalMeleeDamage = MeleeDamage * CurrentDamageMultiplier;

	const float RandomVariance = FMath::RandRange(-0.10f, 0.10f);
	FinalMeleeDamage *= (1.0f + RandomVariance);
	FinalMeleeDamage = FMath::RoundToFloat(FinalMeleeDamage);

	if (UActorComponent* Comp = HitActor->FindComponentByInterface(UDamageableInterface::StaticClass()))
	{
		if (IDamageableInterface* Damageable = Cast<IDamageableInterface>(Comp))
		{
			IDamageableInterface::Execute_ApplyDamage(Comp, FinalMeleeDamage, GetOwner(), CurrentPendingReaction);
			
			if (CurrentExecutingInputType != ECombatInputType::Counter)
			{
				GlobalComboCount++;
				OnCombatInputExecuted.Broadcast(CurrentExecutingInputType, GlobalComboCount);
			}
			
			OnAttackHit.Broadcast(FinalMeleeDamage);
		}
	}

	FAttackDamageEvent DamageEvent;
	DamageEvent.ReactionType = CurrentPendingReaction;
	DamageEvent.DamageTypeClass = UDamageType::StaticClass();

	HitActor->TakeDamage(FinalMeleeDamage, DamageEvent, GetOwner()->GetInstigatorController(), GetOwner());
	InOutAlreadyHitActors.Add(HitActor);
}

void UPlayerCombatComponent::PlayHitEffectsAndSounds(AActor* HitActor, const FHitResult& Hit, bool bIsFirstHit)
{
	AActor* Target = AttackTarget ? AttackTarget : HitActor;

	if (CurrentAttackData)
	{
		// ヒットエフェクトは各対象に再生する
		if (CurrentAttackData->HitVFX && CombatEffectsComp)
		{
			CombatEffectsComp->PlayHitEffect(CurrentAttackData->HitVFX, Hit, Target, CurrentPendingReaction);
		}

		// カメラシェイクと振動は最初のヒット時だけ再生する
		if (bIsFirstHit && CombatEffectsComp)
		{
			CombatEffectsComp->PlayCameraShakeDirect(CurrentAttackData->HitShake);
			
			CombatEffectsComp->PlayHitVibration();
		}
	}

	// ヒット音とイベント通知は最初のヒット時だけ行う
	if (bIsFirstHit)
	{
		if (CurrentAttackSound.Hit)
		{
			UGameplayStatics::PlaySound2D(this, CurrentAttackSound.Hit);
		}
		if (PlayerChar)
		{
			PlayerChar->OnAttackHitSuccess();
		}
	}
	
	if (bIsFirstHit)
	{
		PlayCombatFeedback(CurrentAttackFeedback.Hit);

		if (CurrentAttackSound.Hit)
		{
			UGameplayStatics::PlaySound2D(this, CurrentAttackSound.Hit);
		}
	}
	
}

