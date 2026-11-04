// Copyright 2026 kirzo

#pragma once

#include "Math/MathFwd.h"
#include "Containers/Array.h"

namespace Kz::Geom
{
	KZLIB_API float DistanceToLine(const FVector& A, const FVector& B, const FVector& P);

	/** Decimates a polygon by removing intermediate points that form an angle smaller than the threshold. */
	KZLIB_API TArray<FVector> SimplifyPolygon(TArray<FVector> Polygon, float AngleThreshold = 5.0f);

	KZLIB_API bool IsPointInPolygon2D(const FVector& Point, const TArray<FVector>& Polygon);
	KZLIB_API FVector GetRandomPointInPolygon2D(const TArray<FVector>& Polygon, const FBox& PolygonBounds, int32 MaxAttempts = 100);

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


	// === Cone ===
	// Tip at Origin, opening along local +X. Angles in degrees.

	/** Widest half angle a cone takes: past it the flat cap is unbounded. */
	inline constexpr float MaxConeHalfAngle = 89.0f;

	KZLIB_API FVector ConeSupportPoint(float Length, float HalfAngle, float TipRadius, bool bSphericalCap, const FVector& Direction);
	KZLIB_API FBox ConeBounds(const FVector& Origin, const FQuat& Rotation, float Length, float HalfAngle, float TipRadius, bool bSphericalCap);
	KZLIB_API FVector ClosestPointOnCone(const FVector& Origin, const FQuat& Rotation, float Length, float HalfAngle, float TipRadius, bool bSphericalCap, const FVector& Point);
	KZLIB_API bool ConeIntersectsPoint(const FVector& Origin, const FQuat& Rotation, float Length, float HalfAngle, float TipRadius, bool bSphericalCap, const FVector& Point);

	/** Where the side of a cone meets its cap, as (distance along the axis, distance from it). */
	KZLIB_API FVector2D ConeRim(float Length, float HalfAngle, float TipRadius, bool bSphericalCap);

	/** Outline of a cone in its axial half plane, (along the axis, from it), from the tip's center to the far end of the axis. */
	KZLIB_API void ConeOutline(float Length, float HalfAngle, float TipRadius, bool bSphericalCap, int32 CapSegments, TArray<FVector2D>& OutOutline);


	// === Wedge ===
	// Sector of a cylinder around the local Z axis through Origin, opening along +X. Angles in degrees.

	KZLIB_API FVector WedgeSupportPoint(float Radius, float HalfAngle, float HalfHeight, const FVector& Direction);
	KZLIB_API FBox WedgeBounds(const FVector& Origin, const FQuat& Rotation, float Radius, float HalfAngle, float HalfHeight);
	KZLIB_API FVector ClosestPointOnWedge(const FVector& Origin, const FQuat& Rotation, float Radius, float HalfAngle, float HalfHeight, const FVector& Point);
	KZLIB_API bool WedgeIntersectsPoint(const FVector& Origin, const FQuat& Rotation, float Radius, float HalfAngle, float HalfHeight, const FVector& Point);
}