#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetInterface.generated.h"

UINTERFACE(MinimalAPI)
class UTargetInterface : public UInterface
{
	GENERATED_BODY()
};

/** 攻撃側へ防御判定の受付状態や成功を通知するインターフェース */
class PROJECTJ_API ITargetInterface
{
	GENERATED_BODY()

public:
	// カウンター受付の開始と終了を通知する
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OpenCounterWindow(AActor* Attacker);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void CloseCounterWindow(AActor* Attacker);
	
	// 回避受付の開始と終了を通知する
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OpenDodgeWindow(AActor* Attacker);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void CloseDodgeWindow(AActor* Attacker);
	
	// ジャスト回避受付の開始と終了を通知する
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OpenJustDodgeWindow(AActor* Attacker);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void CloseJustDodgeWindow(AActor* Attacker);
	
	// ジャスト回避成功を防御側へ通知する
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat")
	void OnJustDodgeSuccess(AActor* Defender);
};