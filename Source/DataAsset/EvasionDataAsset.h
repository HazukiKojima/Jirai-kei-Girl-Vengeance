#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Battle/Enum/RelativeDirection.h"
#include "EvasionDataAsset.generated.h"

// 回避1回分のデータ
USTRUCT(BlueprintType)
struct FEvasionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* DodgeMontage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InvincibilityDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DodgeDistanceMultiplier = 1.0f;
};

UCLASS(BlueprintType)
class PROJECTJ_API UEvasionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 方向ごとの回避データマップ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evasion")
	TMap<ERelativeDirection, FEvasionData> DirectionalDodges;
};