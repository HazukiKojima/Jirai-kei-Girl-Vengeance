#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Battle/Enum/RelativeDirection.h"
#include "AttackDataAsset.h"
#include "HitReactionDataAsset.generated.h"

// --- 再生ロジックごとの構造体定義 ---

USTRUCT(BlueprintType)
struct FHitSequenceSet
{
	GENERATED_BODY()
	// Normal(1->2->3) や GroundTakedown(1->2) で使用
	UPROPERTY(EditAnywhere)
	TArray<UAnimMontage*> MontageSequence;
};

USTRUCT(BlueprintType)
struct FHitRandomSet
{
	GENERATED_BODY()
	// Light攻撃などで使用
	UPROPERTY(EditAnywhere)
	TArray<UAnimMontage*> RandomMontages;
};

UCLASS(BlueprintType)
class PROJECTJ_API UHitReactionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// Light: 配列からランダムに再生
	UPROPERTY(EditAnywhere, Category = "Logic | Light")
	FHitRandomSet LightHitSettings;
	
	// Normal
	// やられモーションを一つにまとめたAM
	//UPROPERTY(EditAnywhere, Category = "Logic | Normal")
	//UAnimMontage* NormalHitMontage;

	// Normal: 1 -> 2
	//　今まで通り1が再生し終わったら自動で２を再生、2が再生し終わったら StandUpMontageを再生
	UPROPERTY(EditAnywhere, Category = "Logic | Normal")
	FHitSequenceSet NormalHitSettings;

	// GroundTakedown: 1 -> 2 の順番
	// GroundTakedownは二連コンボ
	// 最初のヒットで１を再生、その次のヒットで２を再生
	// GroundTakedownは確死の攻撃なので2が終わったら死亡する
	UPROPERTY(EditAnywhere, Category = "Logic | Takedown")
	FHitSequenceSet GroundTakedownSettings;

	// Counter: 方向別（前・後・左・右）
	UPROPERTY(EditAnywhere, Category = "Logic | Counter")
	TMap<ERelativeDirection, UAnimMontage*> CounterHitMap;

	// 現状はNormalHitSettingsの２が再生し終わったら、これを再生する
	UPROPERTY(EditAnywhere, Category = "Common")
	UAnimMontage* StandUpMontage;
};