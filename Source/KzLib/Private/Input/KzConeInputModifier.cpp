// Copyright 2026 kirzo

#include "Input/KzConeInputModifier.h"

FVector UKzConeInputModifier::ModifyInput_Implementation(const FVector& OriginalInput, const FVector& CurrentInput)
{
	// Validation: If no input or no valid delegate, pass through.
	if (CurrentInput.IsNearlyZero() || !ReferenceVectorDelegate.IsBound())
	{
		return CurrentInput;
	}

	// Retrieve the dynamic reference vector
	FVector RefVector = ReferenceVectorDelegate.Execute();

	// Prepare vectors for calculation (Planar vs 3D)
	FVector WorkingRef = RefVector;
	FVector WorkingInput = CurrentInput;

	// If planar, zero out Z for the angle check logic
	if (bPlanarConstraint)
	{
		WorkingRef.Z = 0.0f;
		WorkingInput.Z = 0.0f;
	}

	// Safety check after projection
	if (WorkingRef.IsNearlyZero() || WorkingInput.IsNearlyZero())
	{
		return CurrentInput;
	}

	WorkingRef.Normalize();
	const FVector InputDir = WorkingInput.GetSafeNormal();

	const float CosAngle = FVector::DotProduct(InputDir, WorkingRef);
	const float LimitCos = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngle));

	// If we are inside the cone (CosAngle is greater than Limit), we are fine.
	if (CosAngle >= LimitCos)
	{
		return CurrentInput;
	}

	// Calculate the rotation axis (perpendicular to both vectors)
	FVector RotationAxis = FVector::CrossProduct(WorkingRef, InputDir);

	// Handle 180 degree case (collinear opposite) or parallel vectors
	if (RotationAxis.IsNearlyZero())
	{
		// Pick an arbitrary up vector to rotate around
		RotationAxis = FMath::Abs(WorkingRef.Z) < 0.9f ? FVector::UpVector : FVector::RightVector;
	}
	else
	{
		RotationAxis.Normalize();
	}

	// Rotate the Reference vector by the Max Angle towards the Input to find the edge
	FVector ClampedDir = WorkingRef.RotateAngleAxis(ConeHalfAngle, RotationAxis);

	// Reconstruct Result
	if (bPlanarConstraint)
	{
		// We have the clamped 2D direction.
		// Restore the original planar magnitude + the original vertical component.
		FVector FinalResult = ClampedDir * WorkingInput.Size();
		FinalResult.Z = CurrentInput.Z;
		return FinalResult;
	}
	else
	{
		// In 3D, just scale back to original magnitude
		return ClampedDir * CurrentInput.Size();
	}
}