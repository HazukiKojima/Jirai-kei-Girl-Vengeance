#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MotionWarpingComponent.h"
#include "Animation/AnimMontage.h"
#include "../Components/PlayerCombatComponent.h"
#include "Battle/Interfaces/AttackCameraInterface.h"
#include "Battle/Interfaces/CharacterEffectInterface.h"
#include "BtlPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;


/** 入力、カメラ、戦闘コンポーネントをまとめるプレイヤーキャラクター */
UCLASS()
class PROJECTJ_API ABtlPlayerCharacter : public ACharacter , public IAttackCameraInterface, public ICharacterEffectInterface
{
	GENERATED_BODY()

	/** プレイヤーの後方にカメラを配置するスプリングアーム */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** プレイヤーを追従するカメラ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* VroidSkeletalMesh;
	
	/** 大鎌の表示と装着に使うメッシュ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StaticMeshScythe;
	
	/** 攻撃時の位置合わせに使うモーションワーピング */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UMotionWarpingComponent* MotionWarpingComponent;

	/** プレイヤーの戦闘処理 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UPlayerCombatComponent* CombatComponent;
	
	/** ターゲット検索処理 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UTargetingComponent* TargetingComponent;
	
	/** 戦闘演出の再生処理 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UCombatEffectsComponent* CombatEffectsComponent;
	
	/** 先行入力の管理処理 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UInputBufferComponent* InputBufferComponent;
	
	/** 覚醒と装備の切り替え処理 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	class UEquipmentComponent* EquipmentComponent;
	
protected:
	virtual void BeginPlay() override;
	
	// === インターフェイス ===
	virtual void EnterAttackCamera_Implementation(ECombatInputType CombatInputType) override;
	
	virtual void ApplyAwakenOverlay_Implementation(UMaterialInterface* OverlayMaterial) override;
	/** 移動入力 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** 視点操作入力 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;
	
	/** 通常攻撃入力 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AttackNormalAction;
	
	/** アイテム取得入力 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* PickupInteractAction;
	
	/** カウンター入力 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* CounterAction;
	
	/** グラウンドテイクダウン入力 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* GroundTakedownAction;
	
	/** 覚醒入力 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AwakenAction;
	
	/** 回避入力 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* EvadeAction;
public:

	/** キャラクターと各コンポーネントを初期化する */
	ABtlPlayerCharacter();
	/** 戦闘コンポーネントを返す */
	FORCEINLINE class UPlayerCombatComponent* GetCombatComponent() const { return CombatComponent; }
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat")
	void OnAttackHitSuccess();
	
	UFUNCTION(BlueprintNativeEvent, Category="Combat|Camera")
	void EnterNormalAttackCamera();
	virtual void EnterNormalAttackCamera_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, Category="Combat|Camera")
	void EnterComboAttackCamera();
	virtual void EnterComboAttackCamera_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Combat|Camera")
	void EnterCounterAttackCamera();
	virtual void EnterCounterAttackCamera_Implementation();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Combat|Camera")
	void ExitAttackCamera();
	virtual void ExitAttackCamera_Implementation();
	
	/** 攻撃対象へカメラを向ける */
	UFUNCTION(BlueprintNativeEvent, Category="Combat|Camera")
	void FocusCameraOnTarget();
	virtual void FocusCameraOnTarget_Implementation();
	
	UFUNCTION(BlueprintCallable, Category="Combat|Camera")
	void SetScytheVisible(bool bVisible);

protected:

	/** 入力アクションを各操作に割り当てる */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY()
	UMaterialInstanceDynamic* SphereMaskMID;
	
	UFUNCTION(BlueprintCallable, Category="PostProcess")
	void SetSphereMaskAlpha(float Alpha);
protected:

	/** 移動入力を処理する */
	void Move(const FInputActionValue& Value);

	/** 視点入力を処理する */
	void Look(const FInputActionValue& Value);
	
	void OnAttackNormal();
	
	void OnCounterPressed();

	void OnCounterReleased();
	
	void OnGroundTakedown();
	
	void OnAwaken();
	
	void OnEvade();
public:
	
	/** 操作入力から移動を行う */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** 操作入力から視点を動かす */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

public:

	/** カメラアームを返す */
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** 追従カメラを返す */
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** プレイヤーの外見用メッシュを返す */
	FORCEINLINE class USkeletalMeshComponent* GetVroidSkeletalMesh() const { return VroidSkeletalMesh; }
	/** 大鎌のメッシュを返す */
	FORCEINLINE class UStaticMeshComponent* GetScytheStaticMesh() const {return StaticMeshScythe;}
};

