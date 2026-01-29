// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Input/KzInputModifier.h"
#include "KzConeInputModifier.generated.h"

DECLARE_DYNAMIC_DELEGATE_RetVal(FVector, FKzGetVectorDelegate);

/**
 * Restricts the input vector within a cone defined by a reference direction.
 */
UCLASS(Blueprintable, BlueprintType)
class KZLIB_API UKzConeInputModifier : public UKzInputModifier
{
	GENERATED_BODY()

public:
	/**
	 * Delegate to retrieve the current forward vector of the cone.
	 * If not bound, the modifier will have no effect.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Config", meta = (ExposeOnSpawn))
	FKzGetVectorDelegate ReferenceVectorDelegate;

	/** The half-angle of the cone in degrees (e.g., 45 means a 90-degree total spread). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ExposeOnSpawn, ClampMin = "0.0", ClampMax = "180.0"))
	float ConeHalfAngle = 45.0f;

	/**
	 * If true, the cone check and clamping are performed only on the horizontal plane (XY).
	 * The Z component of the input is preserved (e.g., aiming up/down is unaffected, only left/right).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bPlanarConstraint = false;

private:
	virtual FVector ModifyInput_Implementation(const FVector& OriginalInput, const FVector& CurrentInput) override;
};