#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_PlayCameraShake.generated.h"

/** アニメーション通知の発生時にプレイヤーのカメラシェイクを再生する */
UCLASS()
class PROJECTJ_API UAN_PlayCameraShake : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

public:
	// 再生するカメラシェイクのクラス
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	// カメラシェイクの再生強度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float Scale = 1.0f;
};
