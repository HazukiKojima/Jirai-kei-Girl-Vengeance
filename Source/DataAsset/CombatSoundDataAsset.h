#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatSoundDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FAttackSoundSet
{
	GENERATED_BODY()

	// 武器を振る音
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Whoosh = nullptr;

	// ヒット音
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Hit = nullptr;

	// フィニッシュ用
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> FinishHit = nullptr;

	// 攻撃開始時
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
	TObjectPtr<USoundBase> AttackStartVoice = nullptr;
	
	// 攻撃時
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
	TObjectPtr<USoundBase> AttackVoice = nullptr;
};

USTRUCT(BlueprintType)
struct FEvadeSoundSet
{
	GENERATED_BODY()

	// 回避時の掛け声（ボイス）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
	TObjectPtr<USoundBase> Voice = nullptr;

	// 回避動作音（スライディング音、風切り音など）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SE")
	TObjectPtr<USoundBase> MovementSE = nullptr;

	// ジャスト回避成功音
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SE")
	TObjectPtr<USoundBase> JustEvadeSE = nullptr;
};

USTRUCT(BlueprintType)
struct FAwakenSoundSet
{
	GENERATED_BODY()

	// 覚醒開始ボイス
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
	TObjectPtr<USoundBase> StartVoice = nullptr;

	// 覚醒終了ボイス
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
	TObjectPtr<USoundBase> EndVoice = nullptr;
};

UCLASS(BlueprintType)
class PROJECTJ_API UCombatSoundDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	// 強攻撃
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FAttackSoundSet Normal;
	
	// コンボ（1段目、2段目、3段目）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	TArray<FAttackSoundSet> Combo;

	// カウンター
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FAttackSoundSet Counter;

	// 必殺技
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FAttackSoundSet Special;

	// 回避
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Evade")
	FEvadeSoundSet Evade;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Awaken")
	FAwakenSoundSet Awaken;
};