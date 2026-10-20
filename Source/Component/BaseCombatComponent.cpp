#include "Battle/Components/BaseCombatComponent.h"

#include "Kismet/KismetSystemLibrary.h"

UBaseCombatComponent::UBaseCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsDead = false;
}


void UBaseCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ゲーム開始時にHPを満タンにする
	CurrentHp = MaxHp;
}

ERelativeDirection UBaseCombatComponent::CalculateDirectionToTarget(AActor* TargetActor)
{
	// ターゲットが存在しない場合はデフォルトで正面を返す
	if (!TargetActor) return ERelativeDirection::Front;

	AActor* MyOwner = GetOwner();
	if (!MyOwner) return ERelativeDirection::Front;

	// 自分から対象へ向かう水平方向を求める
	FVector DirToTarget = (TargetActor->GetActorLocation() - MyOwner->GetActorLocation()).GetSafeNormal2D();

	// 各方向との内積を比較し、対象に最も近い方角を選ぶ
	float ForwardDot  = FVector::DotProduct(MyOwner->GetActorForwardVector(), DirToTarget);
	float BackwardDot = FVector::DotProduct(-MyOwner->GetActorForwardVector(), DirToTarget);
	float RightDot    = FVector::DotProduct(MyOwner->GetActorRightVector(), DirToTarget);
	float LeftDot     = FVector::DotProduct(-MyOwner->GetActorRightVector(), DirToTarget);

	float MaxDot = ForwardDot;
	ERelativeDirection Result = ERelativeDirection::Front;

	if (BackwardDot > MaxDot) { MaxDot = BackwardDot; Result = ERelativeDirection::Back; }
	if (RightDot > MaxDot)    { MaxDot = RightDot;    Result = ERelativeDirection::Right; }
	if (LeftDot > MaxDot)     { MaxDot = LeftDot;     Result = ERelativeDirection::Left; }

	return Result;
}

void UBaseCombatComponent::OnDeath()
{	
	bIsDead = true;
}

bool UBaseCombatComponent::DoAttackTrace_Implementation(FName DamageSourceBone)
{
	UE_LOG(LogTemp, Warning, TEXT("%s has died."), *GetOwner()->GetName());
	return true;
}

void UBaseCombatComponent::ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType)
{
	UKismetSystemLibrary::PrintString(
		this,
		FString::Printf(TEXT("TakeDamage!")),
		true,
		true,
		FLinearColor::Red,
		2.0f
	);
	// 既に死んでいる、またはダメージが0以下なら何もしない
	if (bIsDead || Damage <= 0.0f) return;

	// HPを減らす（0以下にならないようにClamp）
	CurrentHp = FMath::Clamp(CurrentHp - Damage, 0.0f, MaxHp);

	UE_LOG(LogTemp, Log, TEXT("%s took %f damage. Current HP: %f"), *GetOwner()->GetName(), Damage, CurrentHp);

	// 死亡判定
	if (CurrentHp <= 0.0f)
	{
		OnDeath();
	}
}

void UBaseCombatComponent::ApplyCounterDamage_Implementation(float Damage, AActor* DamageCauser,
	ERelativeDirection HitDirection)
{
	IDamageableInterface::ApplyCounterDamage_Implementation(Damage, DamageCauser, HitDirection);
}

void UBaseCombatComponent::OpenCounterWindow_Implementation(AActor* Attacker)
{
	ITargetInterface::OpenCounterWindow_Implementation(Attacker);
}

void UBaseCombatComponent::CloseCounterWindow_Implementation(AActor* Attacker)
{
	ITargetInterface::CloseCounterWindow_Implementation(Attacker);
}

void UBaseCombatComponent::OpenDodgeWindow_Implementation(AActor* Attacker)
{
	ITargetInterface::OpenDodgeWindow_Implementation(Attacker);
}

void UBaseCombatComponent::CloseDodgeWindow_Implementation(AActor* Attacker)
{
	ITargetInterface::CloseDodgeWindow_Implementation(Attacker);
}

void UBaseCombatComponent::OpenJustDodgeWindow_Implementation(AActor* Attacker)
{
	ITargetInterface::OpenJustDodgeWindow_Implementation(Attacker);
}

void UBaseCombatComponent::CloseJustDodgeWindow_Implementation(AActor* Attacker)
{
	ITargetInterface::CloseJustDodgeWindow_Implementation(Attacker);
}

void UBaseCombatComponent::OnJustDodgeSuccess_Implementation(AActor* Defender)
{
	ITargetInterface::OnJustDodgeSuccess_Implementation(Defender);
}

void UBaseCombatComponent::OnKilledEnemy_Implementation()
{
	IDamageableInterface::OnKilledEnemy_Implementation();
}
