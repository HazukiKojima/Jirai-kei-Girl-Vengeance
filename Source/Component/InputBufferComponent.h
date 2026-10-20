#pragma once

#include "CoreMinimal.h"
#include "Battle/Struct/CombatInputCmd.h"
#include "Components/ActorComponent.h"
#include "InputBufferComponent.generated.h"

/** 有効時間と優先度に基づいて戦闘入力を一時保存するコンポーネント */
UCLASS( ClassGroup=(Combat), meta=(BlueprintSpawnableComponent) )
class PROJECTJ_API UInputBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInputBufferComponent();

	// 入力種別と方向をバッファに登録する
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void BufferInput(ECombatInputType InputType, FVector Direction);

	// 有効期限内の入力が残っているか確認する
	UFUNCTION(BlueprintPure, Category = "Combat|Input")
	bool HasValidBufferedInput() const;

	// 入力を取り出してバッファを空にする
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	FCombatInputCmd ConsumeInput();

	// バッファを空にする
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void ClearBuffer();

protected:
	// 現在保存している入力
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Input")
	FCombatInputCmd CurrentCommand;

	// 入力を保持する秒数
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Input")
	float InputValidTime = 0.5f;

	// 新しい入力で現在の入力を置き換えられるか判定する
	bool ShouldOverrideCurrentInput(ECombatInputType NewInputType) const;
};