// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzTypes.generated.h"

/**
 * 2D Axis flags.
 * Designed to be used as a Bitmask in UPROPERTIES.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EKzAxis2D : uint8
{
	None = 0 UMETA(Hidden),
	X = 1 << 0,
	Y = 1 << 1
};
ENUM_CLASS_FLAGS(EKzAxis2D);

/**
 * 3D Axis flags.
 * Designed to be used as a Bitmask in UPROPERTIES.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EKzAxis3D : uint8
{
	None = 0 UMETA(Hidden),
	X = 1 << 0,
	Y = 1 << 1,
	Z = 1 << 2
};
ENUM_CLASS_FLAGS(EKzAxis3D);