#include "Battle/Components/EquipmentComponent.h"

#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieSceneSequencePlayer.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Battle/Characters/BtlPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UEquipmentComponent::ExecuteAwaken()
{
    if(!bCanAwaken) return;

    if (bIsAwakened) return;
    
    if (!AwakenDataAsset)
    {
       UE_LOG(LogTemp, Warning, TEXT("AwakenDataAsset is not assigned in %s"), *GetName());
       return;
    }
    
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;
    
    UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInstance) return;
    
    // 覚醒開始時の音声を再生する
    if (auto* CombatComp = OwnerPlayer->FindComponentByClass<UPlayerCombatComponent>())
    {
        if (UCombatSoundDataAsset* SoundDA = CombatComp->GetCombatSoundDA())
        {
            CombatComp->PlayCombatSound(SoundDA->Awaken.StartVoice);
        }
    }
    
    // データアセットに登録されたドロー演出を順に再生する
    const TArray<UAnimMontage*>& DrawMontages = AwakenDataAsset->AwakenSet.DrawMontages;
    if (DrawMontages.Num() == 0) return;

    CurrentDrawIndex = 0;
    UAnimMontage* FirstMontage = DrawMontages[CurrentDrawIndex];
    
    if (FirstMontage && !AnimInstance->Montage_IsPlaying(FirstMontage))
    {
        // 覚醒中の移動入力を止め、移動速度をリセットする
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            PC->SetIgnoreMoveInput(true);
        }

        if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
        {
            MovementComp->StopMovementImmediately();
            MovementComp->Velocity = FVector::ZeroVector;
        }

        SetAwakening(true);
        SetAwakened(true);
        OnAwakenStarted.Broadcast();
       
       FOnMontageBlendingOutStarted BlendOutDelegate;
       BlendOutDelegate.BindUObject(this, &UEquipmentComponent::OnDrawMontageBlendingOut);
        
       if (UAnimMontage* CurrentActive = AnimInstance->GetCurrentActiveMontage())
       {
          AnimInstance->Montage_Stop(0.0f, CurrentActive);
       }

        // 覚醒時に先行入力と戦闘フラグを強制リセット
        if (auto* CombatComp = OwnerPlayer->FindComponentByClass<UPlayerCombatComponent>())
        {
            CombatComp->ResetCombatStateForAwaken();
        }
        
       AnimInstance->Montage_Play(FirstMontage);
       AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, FirstMontage);

        if (AwakenLevelSequence)
        {
            FMovieSceneSequencePlaybackSettings Settings;
            Settings.bDisableMovementInput = false;

            ALevelSequenceActor* OutActor = nullptr;
            ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
                GetWorld(),
                AwakenLevelSequence,
                Settings,
                OutActor
            );

            if (SequencePlayer && OutActor)
            {
                // シーケンスの原点をプレイヤーの現在位置に合わせる
                OutActor->bOverrideInstanceData = true;
                if (UDefaultLevelSequenceInstanceData* SeqInstanceData = Cast<UDefaultLevelSequenceInstanceData>(OutActor->DefaultInstanceData))
                {
                    SeqInstanceData->TransformOrigin = OwnerPlayer->GetActorTransform();
                }

                // シーケンス内のPlayerタグを実際のプレイヤーに割り当てる
                OutActor->SetBindingByTag(FName("Player"), { OwnerPlayer });

                SequencePlayer->Play();
            }
        }
        
       ABtlPlayerCharacter* PlayerChar = Cast<ABtlPlayerCharacter>(OwnerCharacter);
       
       if (OwnerPlayer)
       {
          OwnerPlayer->SetScytheVisible(true);

          if (auto* CombatComp = OwnerPlayer->FindComponentByClass<UPlayerCombatComponent>())
          {
             CombatComp->SetAwakenState(true);
          }
          
          if (APlayerController* PC = Cast<APlayerController>(OwnerPlayer->GetController()))
          {
             PC->SetIgnoreMoveInput(true);
          }
       }
    }
}

void UEquipmentComponent::OnDrawMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
    APlayerController* PC = OwnerPlayer ? Cast<APlayerController>(OwnerPlayer->GetController()) : nullptr;
    
    if (bInterrupted) 
    {
        bIsAwakening = false;
        if (PC)
        {
            // 中断時に移動入力の禁止状態を解除する
            PC->ResetIgnoreMoveInput();
        }
        return;
    }

    if (!OwnerPlayer) return;
    
    UAnimInstance* AnimInstance = OwnerPlayer->GetMesh() ? OwnerPlayer->GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInstance) return;

    const TArray<UAnimMontage*>& DrawMontages = AwakenDataAsset->AwakenSet.DrawMontages;
    
    // 再生済みのモンタージュに続けて次の演出へ進む
    CurrentDrawIndex++;
    
    if (DrawMontages.IsValidIndex(CurrentDrawIndex))
    {
        UAnimMontage* NextMontage = DrawMontages[CurrentDrawIndex];
        if (NextMontage)
        {
            FOnMontageBlendingOutStarted BlendOutDelegate;
            BlendOutDelegate.BindUObject(this, &UEquipmentComponent::OnDrawMontageBlendingOut);
            
            AnimInstance->Montage_Play(NextMontage);
            AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, NextMontage);
        }
    }
    else 
    {
        bIsAwakening = false;
        
        if (PC)
        {
            // 重複した入力禁止状態も含めて解除する
            PC->ResetIgnoreMoveInput();
            UE_LOG(LogTemp, Warning, TEXT("Awakening Finished: ResetIgnoreMoveInput Executed"));
        }

        UE_LOG(LogTemp, Warning,
            TEXT("Awaken End | IgnoreMoveInput=%s | MovementMode=%d | Velocity=%s"),
            PC && PC->IsMoveInputIgnored() ? TEXT("TRUE") : TEXT("FALSE"),
            OwnerPlayer->GetCharacterMovement()->MovementMode,
            *OwnerPlayer->GetVelocity().ToString()
        );
    }
}


void UEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerPlayer = Cast<ABtlPlayerCharacter>(GetOwner());
    check(OwnerPlayer);
}

void UEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UEquipmentComponent::ExecuteDeactivateAwaken()
{
    UE_LOG(LogTemp, Log, TEXT("ExecuteDeactivateAwaken (Instant) - Started"));
    if (!OwnerPlayer) return;

    if (auto* CombatComp = OwnerPlayer->FindComponentByClass<UPlayerCombatComponent>())
    {
        if (UCombatSoundDataAsset* SoundDA = CombatComp->GetCombatSoundDA())
        {
            CombatComp->PlayCombatSound(SoundDA->Awaken.EndVoice);
        }
    }
    
    UStaticMeshComponent* ScytheMesh = OwnerPlayer->GetScytheStaticMesh();
    USkeletalMeshComponent* VroidMesh = OwnerPlayer->GetVroidSkeletalMesh();

    if (ScytheMesh && VroidMesh)
    {
        FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
        ScytheMesh->AttachToComponent(VroidMesh, AttachmentRules, FName("Scythe_Back"));
    }
    
    // 大鎌を非表示にする
    OwnerPlayer->SetScytheVisible(false);
    UE_LOG(LogTemp, Log, TEXT("Scythe set to invisible."));

    SetCanAwaken(false);
    SetAwakened(false);
    
    // インターフェース経由でオーバーレイを解除する
    if (OwnerPlayer->GetClass()->ImplementsInterface(UCharacterEffectInterface::StaticClass()))
    {
        ICharacterEffectInterface::Execute_ApplyAwakenOverlay(OwnerPlayer, nullptr);
        UE_LOG(LogTemp, Log, TEXT("ApplyAwakenOverlay(nullptr) executed via Interface safely."));
    }
    else
    {
        VroidMesh->SetOverlayMaterial(nullptr);
        UE_LOG(LogTemp, Log, TEXT("VroidMesh Overlay cleared directly."));
    }

    // 覚醒状態を解除し、通常攻撃を許可する
    if (auto* CombatComp = OwnerPlayer->FindComponentByClass<UPlayerCombatComponent>())
    {
        CombatComp->SetAwakenState(false);
        CombatComp->SetAttackAvailable(true);
        UE_LOG(LogTemp, Log, TEXT("Combat state set to Default, Attack Available set to TRUE."));
    }

    // 移動入力の禁止状態を解除する
    if (APlayerController* PC = Cast<APlayerController>(OwnerPlayer->GetController()))
    {
        PC->SetIgnoreMoveInput(false);
    }
}

void UEquipmentComponent::OnSheatheMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Log, TEXT("OnSheatheMontageBlendingOut Called. Montage: %s, Interrupted: %s"), 
        Montage ? *Montage->GetName() : TEXT("None"), 
        bInterrupted ? TEXT("TRUE") : TEXT("FALSE"));

    if (bInterrupted)
    {
        // 中断時は移動と攻撃の制限を解除する
        if (OwnerPlayer)
        {
            if (APlayerController* PC = Cast<APlayerController>(OwnerPlayer->GetController()))
            {
                PC->SetIgnoreMoveInput(false);
            }
            if (auto* CombatComp = OwnerPlayer->FindComponentByClass<UPlayerCombatComponent>())
            {
                CombatComp->SetAttackAvailable(true);
                UE_LOG(LogTemp, Warning, TEXT("Sheathe Interrupted: SetAttackAvailable(true) executed."));
            }
        }
        return;
    }

    if (!OwnerPlayer) 
    {
        UE_LOG(LogTemp, Warning, TEXT("OnSheatheMontageBlendingOut: OwnerPlayer is null!"));
        return;
    }
    UAnimInstance* AnimInstance = OwnerPlayer->GetMesh() ? OwnerPlayer->GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInstance) 
    {
        UE_LOG(LogTemp, Warning, TEXT("OnSheatheMontageBlendingOut: AnimInstance is null!"));
        return;
    }

    const TArray<UAnimMontage*>& SheatheMontages = AwakenDataAsset->AwakenSet.SheathMontage;
    
    CurrentSheatheIndex++;
    
    UE_LOG(LogTemp, Log, TEXT("Advancing Sheathe Index. Next Index to check: %d / Total Montages: %d"), CurrentSheatheIndex, SheatheMontages.Num());

    // 次の納刀アニメーションがあれば続けて再生する
    if (SheatheMontages.IsValidIndex(CurrentSheatheIndex))
    {
        UAnimMontage* NextMontage = SheatheMontages[CurrentSheatheIndex];
        if (NextMontage)
        {
            UE_LOG(LogTemp, Log, TEXT("Playing next sheathe montage: %s"), *NextMontage->GetName());

            FOnMontageBlendingOutStarted BlendOutDelegate;
            BlendOutDelegate.BindUObject(this, &UEquipmentComponent::OnSheatheMontageBlendingOut);

            AnimInstance->Montage_Play(NextMontage);
            AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, NextMontage);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("NextMontage at index %d is null!"), CurrentSheatheIndex);
        }
    }
    else
    {
        // 全ての納刀アニメーションが終わったら装備と操作状態を戻す
        UE_LOG(LogTemp, Log, TEXT("All sheathe montages finished. Cleaning up states..."));
        
        // 大鎌を非表示にする
        OwnerPlayer->SetScytheVisible(false);

        // 移動入力の禁止状態を解除する
        if (APlayerController* PC = Cast<APlayerController>(OwnerPlayer->GetController()))
        {
            PC->SetIgnoreMoveInput(false);
        }
        
        // 納刀完了後に攻撃を許可する
        if (auto* CombatComp = OwnerPlayer->FindComponentByClass<UPlayerCombatComponent>())
        {
            CombatComp->SetAttackAvailable(true);
            UE_LOG(LogTemp, Log, TEXT("Sheathe Normal End: SetAttackAvailable(true) executed."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to find UPlayerCombatComponent to reset attack state!"));
        }
    }
}