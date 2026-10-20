#pragma once
#include "CoreMinimal.h"
#include "AttackInfo.generated.h"

USTRUCT(BlueprintType)
struct FAttackInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* AttackMontage;
	
	UPROPERTY(EditAnywhere, Category="Animation")
	EHitReactionType HitReaction;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	int32 ComboIndex;
};
