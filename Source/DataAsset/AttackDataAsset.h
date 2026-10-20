#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DamageEvents.h"
#include "AttackDataAsset.generated.h"

// 敵に要求するリアクション強度
UENUM(BlueprintType)
enum class EHitReactionType : uint8
{
	Combo,
	Normal,
	Counter,
	GroundTakedown1,
	GroundTakedown2,
};

USTRUCT(BlueprintType)
struct FAttackDamageEvent : public FDamageEvent
{
	GENERATED_BODY()
 
	// 自分のイベントを識別するためのID
	static const int32 ClassID = 1001;
	virtual int32 GetTypeID() const override { return ClassID; }
 
	// あなたのEnumを直接持たせる
	UPROPERTY()
	EHitReactionType ReactionType;
};

UCLASS(BlueprintType)
class PROJECTJ_API UAttackDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category="Combat")
	bool bIsCounterAsset = false;
	
	UPROPERTY(EditAnywhere, Category="Counter|Animation", meta=(EditCondition="bIsCounterAsset", ToolTip="順番に再生するブロック（受け流し）アニメーション群"))
	TArray<UAnimMontage*> CounterBlockSequence;
	
	UPROPERTY(EditAnywhere, Category="Effects")
	class UNiagaraSystem* HitVFX;

	UPROPERTY(EditAnywhere, Category="Effects")
	USoundBase* HitSound;
	
	UPROPERTY(EditAnywhere, Category="Effects")
	TSubclassOf<class UCameraShakeBase> HitShake;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	EHitReactionType HitReaction = EHitReactionType::Combo;

	UPROPERTY(EditAnywhere, Category="Combat")
	float DamageMultiplier = 1.0f;
};