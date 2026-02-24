// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzMovementTypes.generated.h"

/** Defines how to handle the Z-coordinate (height) of a target location. */
UENUM(BlueprintType)
enum class EKzTargetVerticalAlignment : uint8
{
	/** Uses the target location exactly as provided (including Z). */
	UseTargetZ          UMETA(DisplayName = "Use Target Z"),

	/** Treats the target location as the floor level and adds the character's capsule half-height. */
	AlignFeetToTarget   UMETA(DisplayName = "Align Feet To Target (Add Capsule Height)"),

	/** Ignores the target's Z value and maintains the actor's current Z height. */
	KeepStartZ          UMETA(DisplayName = "Maintain Start Z")
};