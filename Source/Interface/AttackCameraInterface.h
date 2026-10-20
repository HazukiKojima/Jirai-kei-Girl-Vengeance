#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AttackCameraInterface.generated.h"

UINTERFACE(MinimalAPI)
class UAttackCameraInterface : public UInterface
{
	GENERATED_BODY()
};

/** 攻撃カメラ演出の実行先が実装するインターフェース */
class PROJECTJ_API IAttackCameraInterface
{
	GENERATED_BODY()

public:
	// 指定した攻撃種別のカメラ演出を開始する
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Combat|Camera")
	void EnterAttackCamera(ECombatInputType CombatInputType);
};
