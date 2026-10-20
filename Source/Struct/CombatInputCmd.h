#pragma once

#include "CoreMinimal.h"
#include "Battle/DataAsset/CombatDataAsset.h"
#include "CombatInputCmd.generated.h"

USTRUCT(BlueprintType)
struct FCombatInputCmd
{
	GENERATED_BODY()

	// どのアクションか（コンボ、回避、カウンターなど）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ECombatInputType InputType = ECombatInputType::Normal;

	// 入力された時刻（古すぎる入力を破棄するため）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Timestamp = 0.0f;

	// 入力時の左スティックの方向（フリーフローのターゲット選択に必須）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Direction = FVector::ZeroVector;

	// 有効判定
	bool IsValid() const { return Timestamp > 0.0f; }
	void Reset() { Timestamp = 0.0f; Direction = FVector::ZeroVector; }
};