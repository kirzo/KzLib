// Copyright 2025 kirzo

#pragma once

namespace Kz::Geom
{
	// === Sphere ===

	KZLIB_API FBox SphereBounds(const FVector& Center, float Radius);
	KZLIB_API FVector ClosestPointOnSphere(const FVector& Center, float Radius, const FVector& Point);
	KZLIB_API bool SphereIntersectsPoint(const FVector& Center, float Radius, const FVector& Point);


	// === Box ===

	KZLIB_API FBox BoxBounds(const FVector& Center, const FQuat& Rotation, const FVector& HalfSize);
	KZLIB_API FVector ClosestPointOnBox(const FVector& Center, const FQuat& Rotation, const FVector& HalfSize, const FVector& Point);
	KZLIB_API bool BoxIntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& HalfSize, const FVector& Point);


	// === Capsule ===

	KZLIB_API FBox CapsuleBounds(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight);
	KZLIB_API FVector ClosestPointOnCapsule(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point);
	KZLIB_API bool CapsuleIntersectsPoint(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point);


	// === Cylinder ===

	KZLIB_API FBox CylinderBounds(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight);
	KZLIB_API FVector ClosestPointOnCylinder(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point);
	KZLIB_API bool CylinderIntersectsPoint(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point);
}