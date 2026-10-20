#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Battle/Enum/RelativeDirection.h"
#include "Battle/Enum/CombatInputType.h"
#include "AttackDataAsset.h"
#include "EvasionDataAsset.h"
#include "CombatDataAsset.generated.h"

// 攻撃の連なり（コンボ）
USTRUCT(BlueprintType)
struct FAttackSequence
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UAttackDataAsset*> Attacks;
};

// ★ 追加: 1つのコンボグループ（1段目〜フィニッシュまでの攻撃データの並び）
USTRUCT(BlueprintType)
struct FComboGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UAttackDataAsset*> Attacks;
};

// 階層を深くするための「方向ごとのマップ」ラップ
USTRUCT(BlueprintType)
struct FDirectionalActionSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<ECombatInputType, FAttackSequence> ActionMap;
};

UCLASS(BlueprintType)
class PROJECTJ_API UCombatDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ★ 追加: ランダム再生されるコンボグループの配列
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Combo")
	TArray<FComboGroup> ComboGroups;

	// 方向 -> 入力種別 -> 攻撃シーケンス（カウンターや強攻撃などで使用）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	TMap<ERelativeDirection, FDirectionalActionSet> DirectionalActions;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Evasion")
	UEvasionDataAsset* EvasionDataAsset;
};