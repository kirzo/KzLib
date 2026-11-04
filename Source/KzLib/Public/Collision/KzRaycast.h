// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

struct FKzHitResult;

namespace Kz::Raycast
{
	// Fast path: Sphere
	KZLIB_API bool Sphere(FKzHitResult& OutHit, const FVector& Center, float Radius, const FVector& RayStart, const FVector& RayDir, float MaxDistance);

	// Fast path: Box (AABB)
	KZLIB_API bool Box(FKzHitResult& OutHit, const FVector& Center, const FVector& Extents, const FVector& RayStart, const FVector& RayDir, float MaxDistance);

	// Fast path: Box (OBB)
	KZLIB_API bool Box(FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& Extents, const FVector& RayStart, const FVector& RayDir, float MaxDistance);

	// Fast path: Capsule
	KZLIB_API bool Capsule(FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& RayStart, const FVector& RayDir, float MaxDistance);

	// Fast path: Cylinder (aligned in local Z)
	KZLIB_API bool Cylinder(FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& RayStart, const FVector& RayDir, float MaxDistance);
}