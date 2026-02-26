// Copyright 2026 kirzo

#pragma once

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"

/**
 * Pure C++ math utility functions.
 * No UObject overhead, designed to be used natively across the framework.
 */
struct KZLIB_API FKzMath
{
public:
	/**
	 * Calculates the horizontal angle (yaw) between two direction vectors.
	 * @param A First direction vector.
	 * @param B Second direction vector.
	 * @return The horizontal angle in degrees (0 to 180).
	 */
	FORCEINLINE static float GetHorizontalAngle(const FVector& A, const FVector& B)
	{
		const float DotProduct = A.GetSafeNormal2D() | B.GetSafeNormal2D();

		// Clamp the dot product to [-1, 1] to prevent NaN results from floating-point inaccuracies
		const float SafeDot = FMath::Clamp(DotProduct, -1.0f, 1.0f);

		return FMath::RadiansToDegrees(FMath::Acos(SafeDot));
	}

	/**
	 * Calculates the absolute difference in vertical angle between two direction vectors.
	 * @param A First direction vector.
	 * @param B Second direction vector.
	 * @return The vertical angle difference in degrees.
	 */
	FORCEINLINE static float GetVerticalAngleDifference(const FVector& A, const FVector& B)
	{
		// The dot product of a normalized vector and UpVector(0,0,1) is exactly its Z component
		const float DotA = FMath::Clamp(A.GetSafeNormal().Z, -1.0f, 1.0f);
		const float DotB = FMath::Clamp(B.GetSafeNormal().Z, -1.0f, 1.0f);

		const float AngleA = FMath::RadiansToDegrees(FMath::Acos(DotA));
		const float AngleB = FMath::RadiansToDegrees(FMath::Acos(DotB));

		return FMath::Abs(AngleA - AngleB);
	}
};