#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

/** 攻撃、カウンター、テイクダウンの対象を選定するコンポーネント */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTJ_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetingComponent();

	/** 入力方向と距離から攻撃対象を選ぶ */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	AActor* FindBestTarget(const TArray<AActor*>& IgnoredActors, float MaxSearchDistance, AActor* CurrentTarget = nullptr, int32 ComboIndex = 0);
	
	// ダウン中の敵からテイクダウン対象を選ぶ
	AActor* FindBestGroundTakedownTarget(float MaxSearchDistance);

	// カウンター受付中の敵から最も近い対象を選ぶ
	AActor* FindBestCounterTarget(const TSet<TWeakObjectPtr<AActor>>& ActiveCounterWindows);
	
protected:
	virtual void BeginPlay() override;

private:
	/** 対象方向の評価に使う重み */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting|Weight", meta = (AllowPrivateAccess = "true"))
	float DirectionWeight = 1.0f;

	/** 対象距離の評価に使う重み */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting|Weight", meta = (AllowPrivateAccess = "true"))
	float DistanceWeight = 0.5f;
};