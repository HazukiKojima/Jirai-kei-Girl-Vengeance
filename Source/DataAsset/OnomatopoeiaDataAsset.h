#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OnomatopoeiaDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTJ_API UOnomatopoeiaDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UTexture2D>> OnomatopoeiaImages;
};