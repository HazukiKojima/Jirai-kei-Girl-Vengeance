#pragma once

#include "CoreMinimal.h"
#include "RelativeDirection.generated.h"

UENUM(BlueprintType)
enum class ERelativeDirection : uint8
{
	Front,
	Back,
	Left,
	Right,
	Common
};