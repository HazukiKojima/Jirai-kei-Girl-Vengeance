#include "Battle/Components/InputBufferComponent.h"
#include "Battle/DataAsset/CombatDataAsset.h"

UInputBufferComponent::UInputBufferComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInputBufferComponent::BufferInput(ECombatInputType InputType, FVector Direction)
{
	// 保存済み入力が有効な場合は、優先度を確認してから置き換える
	if (HasValidBufferedInput())
	{
		if (!ShouldOverrideCurrentInput(InputType))
		{
			// 優先度が低い入力は破棄する
			return;
		}
	}

	// 入力種別、受付時刻、方向を保存する
	CurrentCommand.InputType = InputType;
	CurrentCommand.Timestamp = GetWorld()->GetTimeSeconds();
	CurrentCommand.Direction = Direction;

	UE_LOG(LogTemp, Log, TEXT("Input Buffered: Type %d"), (int32)InputType);
}

bool UInputBufferComponent::HasValidBufferedInput() const
{
	if (!CurrentCommand.IsValid())
	{
		return false;
	}

	float TimeSinceInput = GetWorld()->GetTimeSeconds() - CurrentCommand.Timestamp;
	return TimeSinceInput <= InputValidTime;
}

FCombatInputCmd UInputBufferComponent::ConsumeInput()
{
	// 期限切れの入力は破棄する
	if (!HasValidBufferedInput())
	{
		ClearBuffer();
		return FCombatInputCmd();
	}

	FCombatInputCmd ConsumedCmd = CurrentCommand;
	ClearBuffer();

	return ConsumedCmd;
}

void UInputBufferComponent::ClearBuffer()
{
	CurrentCommand.Reset();
}

bool UInputBufferComponent::ShouldOverrideCurrentInput(ECombatInputType NewInputType) const
{
	if (NewInputType == ECombatInputType::Counter)
	{
		return true;
	}
	
	if (NewInputType == CurrentCommand.InputType)
	{
		return true;
	}

	return false;
}