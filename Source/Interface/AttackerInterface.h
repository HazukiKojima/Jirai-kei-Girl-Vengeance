#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AttackerInterface.generated.h"

UINTERFACE(MinimalAPI)
class UAttackerInterface : public UInterface
{
	GENERATED_BODY()
};

/** 攻撃判定を実行するオブジェクトが実装するインターフェース */
class PROJECTJ_API IAttackerInterface
{
	GENERATED_BODY()

public:
	// 指定した骨を起点に攻撃トレースを行う
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	bool DoAttackTrace(FName DamageSourceBone);
};
