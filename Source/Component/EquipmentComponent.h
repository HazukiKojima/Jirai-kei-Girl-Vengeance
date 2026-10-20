#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Battle/DataAsset/AwakenDataAsset.h"
#include "EquipmentComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAwakenStarted);

class ABtlPlayerCharacter;
class ULevelSequence;

/** 覚醒の開始・終了と武器や演出の切り替えを管理するコンポーネント */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTJ_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()
		
	
public:	
	// 覚醒コンポーネントを初期化する
	UEquipmentComponent();

	UFUNCTION(BlueprintCallable, Category="Combat")
	void ExecuteAwaken();

	UFUNCTION()
	void OnDrawMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awaken|Material")
	UMaterialInterface* PowerUpOverlayMaterial;
	
	UPROPERTY(BlueprintAssignable, Category="Awakening")
	FOnAwakenStarted OnAwakenStarted;
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awaken|Camera")
	TObjectPtr<ULevelSequence> AwakenLevelSequence;
	
	UPROPERTY()
	ABtlPlayerCharacter* OwnerPlayer = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAwakenDataAsset* AwakenDataAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awaken|Effects")
	class UMaterialInterface* AwakenOverlayMaterial;
public:	
	// フレーム更新
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 覚醒演出の再生中かどうか
	bool bIsAwakening = false;
	UFUNCTION(BlueprintCallable, Category = "Awakening")
	void SetAwakening(bool bNewAwakening) {bIsAwakening = bNewAwakening;}
	
	// 覚醒状態かどうか
	bool bIsAwakened = false;
	UFUNCTION(BlueprintCallable, Category = "Awakening")
	void SetAwakened(bool bNewAwakened) {bIsAwakened = bNewAwakened;}
	
	UFUNCTION(BlueprintPure, Category = "Awakening")
	FORCEINLINE bool IsAwakened() const { return bIsAwakened; }

	// 覚醒を開始できる状態かどうか
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Awakening")
    bool bCanAwaken = false;
	
	UFUNCTION(BlueprintCallable, Category = "Awakening")
	void SetCanAwaken(bool bNewCanAwaken) {bCanAwaken=bNewCanAwaken;}
	
	// 覚醒を終了して通常状態へ戻す
	UFUNCTION(BlueprintCallable, Category = "Battle|Awaken")
	void ExecuteDeactivateAwaken();
private:
	
	UPROPERTY(EditAnywhere)
	int32 CurrentDrawIndex = 0;
	// 再生中の納刀アニメーションの位置
	int32 CurrentSheatheIndex = 0;

	// 納刀アニメーションの終了時に次の処理へ進む
	void OnSheatheMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
};
