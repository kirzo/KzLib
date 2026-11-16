// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

struct FKzShapeInstance;
struct FKzHitResult;

namespace Kz::GJK
{
	/** Performs a raycast against a convex shape. */
	KZLIB_API bool Raycast(FKzHitResult& OutHit,
							 const FVector& RayOrigin, const FVector& RayDir, float MaxDistance,
							 const FKzShapeInstance& Shape, const FVector& ShapePosition, const FQuat& ShapeRotation);

	/** Performs a GJK intersection test between two convex shapes. */
	KZLIB_API bool Intersect(const FKzShapeInstance& ShapeA, const FVector& PositionA, const FQuat& RotationA,
								 const FKzShapeInstance& ShapeB, const FVector& PositionB, const FQuat& RotationB,
								 int32 MaxIterations = 20);

	/** Performs a GJK intersection test between ShapeB and ShapeA moved from StartA to EndA without turning, like an engine sweep. */
	KZLIB_API bool SweepIntersect(const FKzShapeInstance& ShapeA, const FVector& StartA, const FVector& EndA, const FQuat& RotationA,
								 const FKzShapeInstance& ShapeB, const FVector& PositionB, const FQuat& RotationB,
								 int32 MaxIterations = 20);
}