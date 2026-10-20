#include "Battle/AnimNotify/ANS_InputBufferWindow.h"
#include "Battle/Components/PlayerCombatComponent.h"
#include "GameFramework/Character.h"

void UANS_InputBufferWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		// 所有者の戦闘コンポーネントで入力受付を開始する
		if (UPlayerCombatComponent* CombatComp = MeshComp->GetOwner()->FindComponentByClass<UPlayerCombatComponent>())
		{
			CombatComp->OpenInputBuffer();
		}
	}
}

void UANS_InputBufferWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		// 所有者の戦闘コンポーネントで入力受付を終了する
		// 保存済み入力の処理は戦闘コンポーネントに任せる
		if (UPlayerCombatComponent* CombatComp = MeshComp->GetOwner()->FindComponentByClass<UPlayerCombatComponent>())
		{
			CombatComp->CloseInputBuffer();
		}
	}
}
