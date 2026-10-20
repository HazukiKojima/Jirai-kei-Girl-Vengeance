#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/ForceFeedbackEffect.h"
#include "CombatFeedbackDataAsset.generated.h"

// 攻撃アクション用の振動セット
USTRUCT(BlueprintType)
struct FAttackFeedbackSet
{
    GENERATED_BODY()

    // 攻撃実行時（素振りや予備動作）の振動
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UForceFeedbackEffect> Swing = nullptr;

    // ヒット時の振動
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UForceFeedbackEffect> Hit = nullptr;

    // フィニッシュヒット（トドメ）時の強振動
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UForceFeedbackEffect> FinishHit = nullptr;
};

// 回避用の振動セット
USTRUCT(BlueprintType)
struct FEvadeFeedbackSet
{
    GENERATED_BODY()

    // 通常回避動作の振動
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UForceFeedbackEffect> Movement = nullptr;

    // ジャスト回避成功時の特有の振動
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UForceFeedbackEffect> JustEvade = nullptr;
};

// 被弾用の振動セット
USTRUCT(BlueprintType)
struct FDamageFeedbackSet
{
    GENERATED_BODY()

    // 軽攻撃被弾時の振動
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UForceFeedbackEffect> LightHit = nullptr;

    // 重攻撃・吹っ飛び被弾時の振動
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UForceFeedbackEffect> HeavyHit = nullptr;
};

/**
 * 戦闘時の各種バイブレーション（フォースフィードバック）を一括管理するDataAsset
 */
UCLASS(BlueprintType)
class PROJECTJ_API UCombatFeedbackDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    // 通常/強攻撃
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    FAttackFeedbackSet Normal;

    // コンボ攻撃（1段目、2段目、3段目）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    TArray<FAttackFeedbackSet> Combo;

    // カウンター攻撃
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    FAttackFeedbackSet Counter;

    // 必殺技 / テイクダウン
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    FAttackFeedbackSet Special;

    // 回避
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Evade")
    FEvadeFeedbackSet Evade;

    // 被弾
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Damage")
    FDamageFeedbackSet Damage;
};