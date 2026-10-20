// Copyright Epic Games, Inc. All Rights Reserved.

#include "Battle/Characters/BtlPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Battle/Characters/BtlEnemyCharacter.h"
#include "Battle/Components/EquipmentComponent.h"
#include "EngineUtils.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"

void ABtlPlayerCharacter::EnterAttackCamera_Implementation(ECombatInputType CombatInputType)
{
	IAttackCameraInterface::EnterAttackCamera_Implementation(CombatInputType);
}

void ABtlPlayerCharacter::ApplyAwakenOverlay_Implementation(UMaterialInterface* OverlayMaterial)
{
	USkeletalMeshComponent* VroidMesh = GetVroidSkeletalMesh();
	if (VroidMesh)
	{
		VroidMesh->SetOverlayMaterial(OverlayMaterial);
	}
}

ABtlPlayerCharacter::ABtlPlayerCharacter()
{
	// カプセルのサイズを設定する
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// コントローラーの回転はカメラに反映し、キャラクターの向きには使わない
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	Tags.Add("Player");
	
	// 移動方向へキャラクターを向ける
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	VroidSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VroidSkeletalMesh"));
	VroidSkeletalMesh->SetupAttachment(GetMesh());

	StaticMeshScythe = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Scythe"));
	StaticMeshScythe->SetupAttachment(VroidSkeletalMesh, TEXT("Scythe_Back"));
	
	// 攻撃時の位置合わせに使う
	MotionWarpingComponent=CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	CombatComponent = CreateDefaultSubobject<UPlayerCombatComponent>(TEXT("PlayerCombatComponent"));
	
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
	
	CombatEffectsComponent = CreateDefaultSubobject<UCombatEffectsComponent>(TEXT("CombatEffectsComponent"));
	
	InputBufferComponent = CreateDefaultSubobject<UInputBufferComponent>(TEXT("InputBufferComponent"));
	
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
	
	// 移動パラメーターはキャラクターブループリントから調整できる
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// 障害物に応じてカメラ距離を調整するアームを作成する
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// アームに追従するカメラを作成する
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
}

void ABtlPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	SetScytheVisible(false);


	// ポストプロセス設定からSphereMaskマテリアルを探して動的化する
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		APostProcessVolume* PPVolume = *It;

		if (!PPVolume)
			continue;


		TArray<FWeightedBlendable>& Blendables =
			PPVolume->Settings.WeightedBlendables.Array;


		for (FWeightedBlendable& Blendable : Blendables)
		{
			UMaterialInterface* Material =
				Cast<UMaterialInterface>(Blendable.Object);


			if (!Material)
				continue;


			UE_LOG(LogTemp, Warning,
				TEXT("PostProcess Material : %s"),
				*Material->GetName());


			if (Material->GetName().Contains(TEXT("SphereMask")))
			{
				SphereMaskMID =
					UMaterialInstanceDynamic::Create(
						Material,
						this
					);


				Blendable.Object = SphereMaskMID;


				UE_LOG(LogTemp, Warning,
					TEXT("SphereMask MID Created"));


				break;
			}
		}


		if (SphereMaskMID)
			break;
	}
}

void ABtlPlayerCharacter::EnterNormalAttackCamera_Implementation()
{
}

void ABtlPlayerCharacter::EnterComboAttackCamera_Implementation()
{
}

void ABtlPlayerCharacter::EnterCounterAttackCamera_Implementation()
{
}

void ABtlPlayerCharacter::ExitAttackCamera_Implementation()
{
}


void ABtlPlayerCharacter::FocusCameraOnTarget_Implementation()
{
}

void ABtlPlayerCharacter::SetScytheVisible(bool bVisible)
{
	if (StaticMeshScythe)
	{
		StaticMeshScythe->SetVisibility(bVisible);
		StaticMeshScythe->SetHiddenInGame(!bVisible);
	}
}

void ABtlPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 入力アクションを各操作に割り当てる
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		// 移動と視点操作
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABtlPlayerCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ABtlPlayerCharacter::Look);
		EnhancedInputComponent->BindAction(AttackNormalAction, ETriggerEvent::Started, this, &ABtlPlayerCharacter::OnAttackNormal);
		EnhancedInputComponent->BindAction(CounterAction, ETriggerEvent::Started, this, &ABtlPlayerCharacter::OnCounterPressed);
		EnhancedInputComponent->BindAction(GroundTakedownAction, ETriggerEvent::Started, this, &ABtlPlayerCharacter::OnGroundTakedown);
		EnhancedInputComponent->BindAction(AwakenAction, ETriggerEvent::Started, this, &ABtlPlayerCharacter::OnAwaken);
		EnhancedInputComponent->BindAction(EvadeAction, ETriggerEvent::Started, this, &ABtlPlayerCharacter::OnEvade);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ABtlPlayerCharacter::SetSphereMaskAlpha(float Alpha)
{
	UE_LOG(LogTemp, Warning, TEXT("Call SetSphereMaskAlpha"));

	if (SphereMaskMID)
	{
		UE_LOG(LogTemp, Warning, TEXT("MID OK"));

		SphereMaskMID->SetScalarParameterValue(TEXT("Alpha"), Alpha);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SphereMaskMID == nullptr"));
	}
}

void ABtlPlayerCharacter::Move(const FInputActionValue& Value)
{
	// 2D入力を移動量に変換する
	FVector2D MovementVector = Value.Get<FVector2D>();

	DoMove(MovementVector.X, MovementVector.Y);
}

void ABtlPlayerCharacter::Look(const FInputActionValue& Value)
{
	// 2D入力を視点の回転量に変換する
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ABtlPlayerCharacter::OnAttackNormal()
{
	if (CombatComponent)
	{
		CombatComponent->TryExecuteNormalAttack();
	}
}

void ABtlPlayerCharacter::OnCounterPressed()
{
	if (CombatComponent)
	{
		CombatComponent->OnCounterPressed();
	}
}

void ABtlPlayerCharacter::OnCounterReleased()
{
	if (CombatComponent)
	{
		CombatComponent->OnCounterReleased();
	}
}

void ABtlPlayerCharacter::OnGroundTakedown()
{
	if (CombatComponent)
	{
		CombatComponent->ExecuteGroundTakedown();
	}
}

void ABtlPlayerCharacter::OnAwaken()
{
	if (EquipmentComponent)
	{
		EquipmentComponent->ExecuteAwaken();
	}
}

void ABtlPlayerCharacter::OnEvade()
{
	if (CombatComponent)
	{
		CombatComponent->TryEvade();
	}
}

void ABtlPlayerCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// コントローラーの向きを基準に移動方向を求める
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ABtlPlayerCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// コントローラーに視点の回転入力を加える
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}