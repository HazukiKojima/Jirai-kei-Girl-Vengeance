#include "Battle/AnimNotify/AN_PlayCameraShake.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UAN_PlayCameraShake::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !CameraShakeClass) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	APawn* Pawn = Cast<APawn>(Owner);
	if (!Pawn) return;

	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	// カメラシェイクはローカルプレイヤーでのみ再生する
	if (PC && PC->IsLocalController())
	{
		PC->ClientStartCameraShake(CameraShakeClass, Scale);
	}
}