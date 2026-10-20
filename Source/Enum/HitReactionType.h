#pragma once

#include "CoreMinimal.h"
#include "HitReactionType.generated.h"

UENUM(BlueprintType)
enum class HitReaction : uint8
{
	Light,
	Medium,
	Heavy,
	Launch,
	Counter,
	Special
};