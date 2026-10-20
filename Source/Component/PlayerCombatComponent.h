#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseCombatComponent.h"
#include "Battle/DataAsset/AttackDataAsset.h"
#include "Battle/DataAsset/CombatDataAsset.h"
#include "Battle/DataAsset/CombatSoundDataAsset.h"
#include "TargetingComponent.h"
#include "CombatEffectsComponent.h"
#include "InputBufferComponent.h"
#include "Battle/DataAsset/CombatFeedbackDataAsset.h"
#include "PlayerCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackHitSignature, float, Damage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnCombatInputExecutedSignature,
    ECombatInputType, InputType,
    int32, ComboCount
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboResetSignature);

class AWoodenCrate;
class ABtlPlayerCharacter;
class UContextualAnimSceneAsset;
class UCombatFeedbackDataAsset;

/** プレイヤーの攻撃、回避、カウンターと先行入力を管理する戦闘コンポーネント */
UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTJ_API UPlayerCombatComponent : public UBaseCombatComponent
{
    GENERATED_BODY()

    UPROPERTY()
    class UCombatEffectsComponent* CombatEffectsComp;

    UPROPERTY()
    class UInputBufferComponent* InputBufferComp;

public:
    UPlayerCombatComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // -----------------------------------------------------------------
    // イベント
    // -----------------------------------------------------------------
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnAttackHitSignature OnAttackHit;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatInputExecutedSignature OnCombatInputExecuted;
    
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnComboResetSignature OnComboReset;

    // -----------------------------------------------------------------
    // 先行入力システム (Input Buffering System)
    // -----------------------------------------------------------------
    // 外部（PlayerCharacterのInputバインドなど）から呼ばれる攻撃実行の入り口
    void TryExecuteNormalAttack();

    // アニメーション通知から受付区間を制御する
    void OpenInputBuffer();
    void CloseInputBuffer();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnCounterPressed();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnCounterReleased();

    // -----------------------------------------------------------------
    // 戦闘アクション
    // -----------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ExecuteGroundTakedown();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void TryCounter();

    // 覚醒状態を設定する
    void SetAwakenState(bool bNewAwaken);
    
    // 覚醒開始時に戦闘状態と先行入力をリセットする
    void ResetCombatStateForAwaken();
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void TryEvade();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ExecuteEvade(FVector InputDirection = FVector::ZeroVector);

    UFUNCTION(BlueprintCallable, Category = "Combat|Special Move")
    void ExecuteSpecialMove();

    // 無敵時間の開始と終了を設定する
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void SetEvadeState(bool bIsInvincible);

    // -----------------------------------------------------------------
    // 状態の取得
    // -----------------------------------------------------------------
    UFUNCTION(BlueprintPure, Category = "Combat")
    FORCEINLINE ECombatInputType GetCurrentCombatInputType() const { return CurrentExecutingInputType; }

    UFUNCTION(BlueprintPure, Category = "Combat|State")
    int32 GetComboIndex() const;
    
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    int32 GetGlobalComboCount() const { return GlobalComboCount; }
    
    UFUNCTION(BlueprintCallable, Category = "Combat|State")
    void SetGlobalComboCount(int32 NewComboCount);
    
    UFUNCTION(BlueprintPure, Category = "Combat|Sound")
    UCombatSoundDataAsset* GetCombatSoundDA() const { return CombatSoundDA; }
    
    // -----------------------------------------------------------------
    // 演出とターゲットの設定
    // -----------------------------------------------------------------
    
    // 戦闘音声を再生する
    void PlayCombatSound(USoundBase* SoundToPlay);
    
    void PlayCombatFeedback(UForceFeedbackEffect* FeedbackToPlay);
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void SetAttackTarget(AActor* NewTarget) { AttackTarget = NewTarget; }

protected:
    virtual void BeginPlay() override;

    // -----------------------------------------------------------------
    // 攻撃実行
    // -----------------------------------------------------------------
    void ExecuteNormalAttack(FVector InputDirection = FVector::ZeroVector);
    bool CheckAndExecuteGroundTakedownConditional();

    // -----------------------------------------------------------------
    // 基本戦闘処理のオーバーライド
    // -----------------------------------------------------------------
    virtual bool DoAttackTrace_Implementation(FName DamageSourceBone) override;
    virtual void ApplyDamage_Implementation(float Damage, AActor* DamageCauser, EHitReactionType HitReactionType) override;
    virtual void OnDeath() override;

    // -----------------------------------------------------------------
    // カウンター
    // -----------------------------------------------------------------
    virtual void OpenCounterWindow_Implementation(AActor* Attacker) override;
    virtual void CloseCounterWindow_Implementation(AActor* Attacker) override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Counter")
    void ExecuteCounter(AActor* Target);

    void OnCounterBlockMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

    // -----------------------------------------------------------------
    // 回避
    // -----------------------------------------------------------------

    // ジャスト回避判定を受け付けている敵
    UPROPERTY()
    TArray<AActor*> ActiveJustDodgeWindows;
    
    virtual void OpenJustDodgeWindow_Implementation(AActor* Attacker) override;
    virtual void CloseJustDodgeWindow_Implementation(AActor* Attacker) override;
    
    // -----------------------------------------------------------------
    // ヒット判定と演出
    // -----------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ProcessHitResults(const TArray<FHitResult>& OutHits);

    UFUNCTION(BlueprintCallable, Category = "Combat|Collision")
    void SetTakedownCollisionEnabled(bool bIgnorePawn);

    void PlayEffect(const FHitResult& Hit);
    void PlayEffect(AActor* TargetActor);
    void UpdateWarpTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void NotifyAttackHit(EHitReactionType HitReactionType);
    
    // -----------------------------------------------------------------
    // ターゲット選択
    // -----------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Target")
    UAnimMontage* SelectAnimationByDistance();

    void CleanupInvalidCounters();
    UAnimMontage* SelectCounterMontageByDir(ERelativeDirection Dir);
    UAnimMontage* SelectCounterMontage(AActor* Target);

    UPROPERTY()
    UTargetingComponent* TargetingComp;

    // -----------------------------------------------------------------
    // アニメーション通知
    // -----------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    void OnMontageStarted(UAnimMontage* Montage);

    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

public:
    FORCEINLINE bool IsAttackAvailable() const { return bAttackAvailable; }

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void SetAttackAvailable(bool bEnable);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Visuals")
    TObjectPtr<UNiagaraSystem> ReceiveDamageVFX;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
    TObjectPtr<USoundBase> ReceiveDamageSound;
    // -----------------------------------------------------------------
    // 先行入力状態
    // -----------------------------------------------------------------
    bool bIsBufferOpen = false;

    // -----------------------------------------------------------------
    // 戦闘状態とキャッシュ
    // -----------------------------------------------------------------
    UPROPERTY()
    UAttackDataAsset* CurrentAttackData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Sound")
    UCombatSoundDataAsset* CombatSoundDA;

    UPROPERTY()
    FAttackSoundSet CurrentAttackSound;

    UPROPERTY()
    FAttackFeedbackSet CurrentAttackFeedback;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Feedback")
    TObjectPtr<UCombatFeedbackDataAsset> CombatFeedbackDA;;
    
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Cache")
    EHitReactionType CurrentPendingReaction;

    UPROPERTY(BlueprintReadOnly, Category = "Combat|Cache")
    float CurrentDamageMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
    int32 CurrentComboIndex = 0;
    
    // 被弾でリセットされるまで継続するコンボ数
    UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
    int32 GlobalComboCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
    int32 CurrentComboGroupIndex = 0;

    int32 CurrentCounterBlockIndex = 0;
    bool bIsAwakened = false;
    ECombatInputType CurrentExecutingInputType;
    ERelativeDirection CurrentComboDirection;
    ECombatInputType LastInputType = ECombatInputType::Normal;

    UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
    bool bIsWaitingForCounterHit = false;

    UPROPERTY()
    TSet<TWeakObjectPtr<AActor>> ActiveCounterWindows;

    UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
    bool bIsEvading = false;

    // -----------------------------------------------------------------
    // ターゲット設定
    // -----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    bool bAttackAvailable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float BestTargetDistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float MaxTargetDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float CloseRangeThreshold = 500.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    AActor* AttackTarget;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    AActor* FirstAttackTarget;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    AActor* SecondAttackTarget;

    virtual void OnKilledEnemy_Implementation() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Invincibility")
    float HitInvincibilityDuration = 1.5f;
    // -----------------------------------------------------------------
    // 武器・オブジェクトデータ (Weapon & Object Properties)
    // -----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    AWoodenCrate* ClosestCrate;

    // -----------------------------------------------------------------
    // 各種データアセット類 (Data Assets)
    // -----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Data")
    UCombatDataAsset* CombatDataAsset;

    UPROPERTY(EditAnywhere, Category = "Combat|Data")
    UCombatDataAsset* DefaultCombatDataAsset;

    UPROPERTY(EditAnywhere, Category = "Combat|Data")
    UCombatDataAsset* AwakenCombatDataAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Special Move")
    UContextualAnimSceneAsset* SpecialMoveSceneAsset;
    
    // -----------------------------------------------------------------
    // アニメーション資産 (Animation Assets)
    // -----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
    UAnimMontage* DeathMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    UAnimMontage* PickupMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
    TObjectPtr<UAnimMontage> LightHitMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
    TObjectPtr<UAnimMontage> NormalHitMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Punch")
    TArray<UAnimMontage*> AttackPunchFrontMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Punch")
    TArray<UAnimMontage*> AttackPunchBackMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Punch")
    TArray<UAnimMontage*> AttackPunchLeftMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Punch")
    TArray<UAnimMontage*> AttackPunchRightMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Kick")
    TArray<UAnimMontage*> AttackKickFrontMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Kick")
    TArray<UAnimMontage*> AttackKickBackMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Kick")
    TArray<UAnimMontage*> AttackKickLeftMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Kick")
    TArray<UAnimMontage*> AttackKickRightMontage;

    UPROPERTY(EditAnywhere, Category = "Combat|Counter")
    UAnimMontage* CounterMontageFront;
    UPROPERTY(EditAnywhere, Category = "Combat|Counter")
    UAnimMontage* CounterMontageBack;
    UPROPERTY(EditAnywhere, Category = "Combat|Counter")
    UAnimMontage* CounterMontageLeft;
    UPROPERTY(EditAnywhere, Category = "Combat|Counter")
    UAnimMontage* CounterMontageRight;

private:
    UPROPERTY()
    ABtlPlayerCharacter* PlayerChar = nullptr;

    UPROPERTY()
    TWeakObjectPtr<AActor> LastHitTarget;

    bool bIsCounterRequestHeld = false;

    FTimerHandle CounterTimerHandle;

    UPROPERTY(EditAnywhere, Category = "Counter")
    float CounterDuration = 1.0f;

    void OnCounterTimeout();

    // 戦闘状態の判定
    bool IsOwnerAwakeningAnimationBlocked() const;
    bool BufferCurrentInputIfOpen(ECombatInputType InputType) const;
    bool IsDamageImmuned() const;

    // 被弾後の無敵状態
    bool bIsHitInvincible = false;
    
    // 被弾後の無敵時間を管理するタイマー
    FTimerHandle HitInvincibilityTimerHandle;

    // 被弾後の無敵状態を解除する
    void ClearHitInvincibility();

    // 攻撃実行の内部処理
    void UpdateWarpTargetToAttackTarget();
    AActor* DetermineAttackTarget();
    ECombatInputType DetermineCombatInputType(AActor* Target);
    UAttackDataAsset* GetAttackDataForInput(ECombatInputType InputType, int32& OutIndex);
    void PlayAttackSequence(UAttackDataAsset* AttackData, ECombatInputType InputType, int32 Index, AActor* Target);

    // カウンター処理
    void CancelComboForCounter(UAnimInstance* AnimInstance);
    void PlayCounterBlockMontageWithDelegate(UAnimInstance* AnimInstance, UAnimMontage* Montage);
    bool HandleCounterHitIfWaiting(UAnimInstance* AnimInstance);
    void TryBufferOrCancelComboForCounter(AActor* CounterTarget);
    void ExecuteCounterStandby(AActor* CounterTarget);

    // 回避処理
    void TryBufferOrCancelComboForEvade(FVector InputDirection);
    
    // ダメージとヒット処理
    void HandleDamageReaction();
    void ApplyMeleeDamageToTarget(AActor* HitActor, const FHitResult& Hit, TArray<AActor*>& InOutAlreadyHitActors);
    void PlayHitEffectsAndSounds(AActor* HitActor, const FHitResult& Hit, bool bIsFirstHit);
};
