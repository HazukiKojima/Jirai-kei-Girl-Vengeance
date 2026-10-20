#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Battle/DataAsset/CombatSoundDataAsset.h"
#include "GameFramework/ForceFeedbackEffect.h"
#include "CombatEffectsComponent.generated.h"

struct FAttackSoundData;
class UCombatSoundDataAsset;
class UNiagaraSystem;
class UMaterialInterface;

/** 戦闘中のカメラ、音声、振動、視覚効果を再生するコンポーネント */
UCLASS( ClassGroup=(Combat), meta=(BlueprintSpawnableComponent) )
class PROJECTJ_API UCombatEffectsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatEffectsComponent();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Vibration")
	TObjectPtr<UForceFeedbackEffect> HitForceFeedback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Vibration")
	TObjectPtr<UForceFeedbackEffect> CounterForceFeedback;
public:	
	// -----------------------------------------------------------------
	// コントローラー振動
	// -----------------------------------------------------------------
	void PlayHitVibration();
	void PlayCounterVibration();
	
	// -----------------------------------------------------------------
	// カメラアシスト
	// -----------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Combat|Camera")
	void StartCameraAssist(AActor* Target);

	void UpdateCameraAssist();

	UPROPERTY(EditAnywhere, Category = "Combat|Camera")
	float CameraAssistDuration = 0.3f; 

	UPROPERTY(EditAnywhere, Category = "Combat|Camera")
	float MaxCameraAssistAngle = 60.0f;

private:
	FTimerHandle CameraAssistTimerHandle;
	TWeakObjectPtr<AActor> CurrentCameraAssistTarget;
	float CameraAssistElapsedTime = 0.0f;

public:
	// -----------------------------------------------------------------
	// カメラシェイク & エフェクト
	// -----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Camera Shake")
	TSubclassOf<UCameraShakeBase> ReceiveDamageCameraShake;
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Camera")
	void PlayReceiveDamageCameraShake();

	UFUNCTION(BlueprintCallable, Category="Combat|Camera")
	void PlayCameraShakeAtLocation(TSubclassOf<UCameraShakeBase> ShakeClass, const FVector& Location);

	UFUNCTION(BlueprintCallable, Category="Combat|Camera")
	void PlayCameraShakeDirect(TSubclassOf<UCameraShakeBase> ShakeClass);

	UFUNCTION(BlueprintCallable, Category = "Combat|VFX")
	void PlayHitEffect(UNiagaraSystem* HitVFX, const FHitResult& Hit, AActor* TargetActor, EHitReactionType ReactionType);
	
	UFUNCTION(BlueprintCallable, Category="Combat|VFX")
	void SpawnHitVFX(UNiagaraSystem* HitVFX, const FVector& Location, const FRotator& Rotation);

	// コンバットカメラ（インターフェース実行）の発火
	UFUNCTION(BlueprintCallable, Category = "Combat|Camera")
	void TriggerAttackCamera(ECombatInputType InputType);
	// -----------------------------------------------------------------
	// サウンド
	// -----------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Combat|Audio")
	void PlayAttackSound(ECombatInputType InputType, int32 ComboIndex, UCombatSoundDataAsset* SoundDA);
	
	// -----------------------------------------------------------------
	// スローモーション (キル時)
	// -----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|SlowMotion")
	float KillTimeDilation = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|SlowMotion")
	float KillSlowDuration = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Combat|SlowMotion")
	void TriggerKillSlowMotion();
	
	// -----------------------------------------------------------------
	// コントローラー振動
	// -----------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Combat|Effects")
	void PlayForceFeedback(UForceFeedbackEffect* ForceFeedback);
	
private:
	FTimerHandle SlowMoTimerHandle;
	
	UFUNCTION()
	void ResetTimeDilation();

public:
	// -----------------------------------------------------------------
	// フィニッシャー演出
	// -----------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Combat|Finisher")
	UMaterialInterface* FinisherOverlayMaterial;

	UPROPERTY(EditAnywhere, Category = "Combat|Finisher")
	float FinisherSlowDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Combat|Finisher")
	float FinisherTimeDilation = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Combat|Finisher")
	float FinisherOverlayDuration = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Combat|Finisher")
	void TriggerFinisherEffect();

private:
	FTimerHandle FinisherTimerHandle;
	FTimerHandle FinisherOverlayTimerHandle;

	UFUNCTION()
	void ResetFinisherEffect();

	UFUNCTION()
	void ResetFinisherOverlay();
};