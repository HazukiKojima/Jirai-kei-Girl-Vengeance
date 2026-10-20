#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_InputBufferWindow.generated.h"

/** 攻撃アニメーション中の先行入力受付区間を制御する通知ステート */
UCLASS()
class PROJECTJ_API UANS_InputBufferWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 通知区間の開始時に先行入力の受付を始める
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	// 通知区間の終了時に先行入力の受付を終える
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};