#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AwakenDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FAwakenSet
{
	GENERATED_BODY()

	/** 抜刀 */
	UPROPERTY(EditAnywhere)
	TArray<UAnimMontage*> DrawMontages;

	/** 納刀 */
	UPROPERTY(EditAnywhere)
	TArray<UAnimMontage*> SheathMontage;
};

UCLASS(BlueprintType, Blueprintable)
class PROJECTJ_API UAwakenDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 抜刀・納刀のモーションセット */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Awaken")
	FAwakenSet AwakenSet;
};