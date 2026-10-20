#pragma once

#include "CoreMinimal.h"
#include "Battle/Enum/RelativeDirection.h"
#include "Components/ActorComponent.h"
#include "Battle/Interfaces/AttackerInterface.h"
#include "Battle/Interfaces/DamageableInterface.h"
#include "Battle/Interfaces/TargetInterface.h"
#include "BaseCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEliminatedSignature);

/** 体力、ダメージ、攻撃判定など共通の戦闘処理を提供する基底コンポーネント */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTJ_API UBaseCombatComponent : public UActorComponent, public IAttackerInterface,
public IDamageableInterface, public ITargetInterface
{
	GENERATED_BODY()

public:	
	// 戦闘コンポーネントを初期化する
	UBaseCombatComponent();

	/** 最大体力に対する現在体力の割合を返す */
	UFUNCTION(BlueprintPure, Category="Status")
	float GetHpPercentage() const { return (MaxHp > 0.f) ? (CurrentHp / MaxHp) : 0.f; }
	
	/** 死亡しているかどうか */
	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsDead() const { return bIsDead; }
	
	// 死亡時に通知するイベント
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category="Events")
	FOnEliminatedSignature OnEliminated;

	UFUNCTION(BlueprintCallable)
	void ForceCallOnEliminated() { OnEliminated.Broadcast(); }
protected:
	virtual void BeginPlay() override;
	
	// 体力
	/** 最大体力 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status")
	float MaxHp = 100.0f;
	
	/** 現在の体力 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status")
	float CurrentHp;
	
	/** 近接攻撃のトレース（当たり判定）の長さ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	float MeleeTraceDistance = 75.0f;
	
	/** 近接攻撃のトレース半径 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	float MeleeTraceRadius = 75.0f;
	
	/** ダメージ量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	float MeleeDamage = 30.0f;
	
	/** カウンターダメージ量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	float CounterDamageValue = 20.0f;
	
	// 戦闘状態
	/** 攻撃中かどうか */
	bool bIsAttacking = false;
	
	ERelativeDirection CalculateDirectionToTarget(AActor* TargetActor);
	
	/** 死亡フラグ */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="State")
	bool bIsDead = false;
	
	/** 死亡時 */
	UFUNCTION(BlueprintCallable, Category="Combat")
	virtual void OnDeath();
	
	// インターフェース実装
	virtual bool DoAttackTrace_Implementation(FName DamageSourceBone) override;
	
	virtual void ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType) override;
	
	virtual void ApplyCounterDamage_Implementation(float Damage, AActor* DamageCauser, ERelativeDirection HitDirection) override;
	
	virtual void OpenCounterWindow_Implementation(AActor* Attacker) override;
	
	virtual void CloseCounterWindow_Implementation(AActor* Attacker) override;
	
	virtual void OpenDodgeWindow_Implementation(AActor* Attacker) override;
	
	virtual void CloseDodgeWindow_Implementation(AActor* Attacker) override;
	
	virtual void OpenJustDodgeWindow_Implementation(AActor* Attacker) override;
	
	virtual void CloseJustDodgeWindow_Implementation(AActor* Attacker) override;
	
	virtual void OnJustDodgeSuccess_Implementation(AActor* Defender) override;
	
	virtual void OnKilledEnemy_Implementation() override;
};
