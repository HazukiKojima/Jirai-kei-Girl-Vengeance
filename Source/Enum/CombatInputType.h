#pragma once

#include "CoreMinimal.h"
#include "CombatInputType.generated.h"

UENUM(BlueprintType)
enum class ECombatInputType : uint8
{
	Combo,
	Normal,
	Special,
	Counter,
	GroundTakedown1,
	GroundTakedown2,
	StepAttack,
	Evade,
};