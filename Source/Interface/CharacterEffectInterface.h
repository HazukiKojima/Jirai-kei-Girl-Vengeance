#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CharacterEffectInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCharacterEffectInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTJ_API ICharacterEffectInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Effects")
	void ApplyAwakenOverlay(UMaterialInterface* OverlayMaterial);
};
