#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

/** ダメージや撃破通知を受け取るオブジェクトが実装するインターフェース */
class PROJECTJ_API IDamageableInterface
{
	GENERATED_BODY()

public:
	// ダメージと被弾反応を適用する
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat")
	void ApplyDamage(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType);
	
	// カウンター攻撃のダメージと方向を適用する
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat")
	void ApplyCounterDamage(float Damage, AActor* DamageCauser, ERelativeDirection HitDirection);
	
	// 敵撃破を通知する
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat")
	void OnKilledEnemy();
};