// Copyright 2026 kirzo

#include "Math/Geometry/KzGeometry.h"
#include "Math/Box.h"
#include "Math/Vector.h"
#include "Math/Quat.h"

namespace Kz::Geom
{
	// === Sphere ===

	FBox SphereBounds(const FVector& Center, float Radius)
	{
		const FVector Extent(Radius);
		return FBox(Center - Extent, Center + Extent);
	}

	FVector ClosestPointOnSphere(const FVector& Center, float Radius, const FVector& Point)
	{
		const FVector LocalPoint = Point - Center;
		const float VSq = LocalPoint.SizeSquared();
		if (VSq <= FMath::Square(Radius)) return Point;
		return Center + LocalPoint * (Radius * FMath::InvSqrt(VSq));
	}

	bool SphereIntersectsPoint(const FVector& Center, float Radius, const FVector& Point)
	{
		return FVector::DistSquared(Point, Center) <= Radius * Radius;
	}


	// === Box ===

	FBox BoxBounds(const FVector& Center, const FQuat& Rotation, const FVector& HalfSize)
	{
		const FVector X = Rotation.GetAxisX().GetAbs() * HalfSize.X;
		const FVector Y = Rotation.GetAxisY().GetAbs() * HalfSize.Y;
		const FVector Z = Rotation.GetAxisZ().GetAbs() * HalfSize.Z;

		const FVector Extent(X.X + Y.X + Z.X, X.Y + Y.Y + Z.Y, X.Z + Y.Z + Z.Z);
		return FBox(Center - Extent, Center + Extent);
	}

	FVector ClosestPointOnBox(const FVector& Center, const FQuat& Rotation, const FVector& HalfSize, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);
		return Center + Rotation.RotateVector(LocalPoint.BoundToBox(-HalfSize, HalfSize));
	}

	bool BoxIntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& HalfSize, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);
		return
			FMath::IsWithinInclusive(LocalPoint.X, -HalfSize.X, HalfSize.X) &&
			FMath::IsWithinInclusive(LocalPoint.Y, -HalfSize.Y, HalfSize.Y) &&
			FMath::IsWithinInclusive(LocalPoint.Z, -HalfSize.Z, HalfSize.Z);
	}


	// === Capsule ===

	FBox CapsuleBounds(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight)
	{
		// Vector from center to one cap center (local +Z axis)
		const FVector Axis = Rotation.GetAxisZ();
		const FVector HalfSegment = Axis * (HalfHeight - Radius);

		// Endpoints of the capsule spine
		const FVector CapA = Center - HalfSegment;
		const FVector CapB = Center + HalfSegment;

		// Compute min/max bounds including the radius
		const FVector Min = FVector::Min(CapA, CapB) - FVector(Radius);
		const FVector Max = FVector::Max(CapA, CapB) + FVector(Radius);

		return FBox(Min, Max);
	}

	FVector ClosestPointOnCapsule(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);
		if (FMath::Abs(LocalPoint.Z) <= HalfHeight - Radius)
		{
			return Center + Rotation.RotateVector(LocalPoint.GetClampedToMaxSize2D(Radius));
		}
		else
		{
			const FVector Offset = FVector::UpVector * FMath::Sign(LocalPoint.Z) * (HalfHeight - Radius);
			return Center + Rotation.RotateVector((LocalPoint - Offset).GetClampedToMaxSize(Radius) + Offset);
		}
	}

	bool CapsuleIntersectsPoint(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);

		return
			(FMath::Abs(LocalPoint.Z) <= HalfHeight - Radius &&
			 LocalPoint.SizeSquared2D() <= FMath::Square(Radius)) ||
			FVector::DistSquared(FVector::UpVector * (HalfHeight - Radius) * FMath::Sign(LocalPoint.Z), LocalPoint) <= FMath::Square(Radius);
	}


	// === Cylinder ===

	FBox CylinderBounds(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight)
	{
		// Axis of the cylinder in world space
		const FVector Axis = Rotation.GetAxisZ();

		// Offset to top/bottom faces
		const FVector HalfSegment = Axis * HalfHeight;

		// Combine caps and radius
		const FVector Top = Center + HalfSegment;
		const FVector Bottom = Center - HalfSegment;

		const FVector Min = FVector::Min(Top, Bottom) - FVector(Radius);
		const FVector Max = FVector::Max(Top, Bottom) + FVector(Radius);

		return FBox(Min, Max);
	}

	FVector ClosestPointOnCylinder(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);

		FVector ClosestPoint = LocalPoint.GetClampedToMaxSize2D(Radius);
		ClosestPoint.Z = FMath::Clamp(LocalPoint.Z, -HalfHeight, HalfHeight);

		return Center + Rotation.RotateVector(ClosestPoint);
	}

	bool CylinderIntersectsPoint(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);
		return FMath::Abs(LocalPoint.Z) <= HalfHeight && LocalPoint.SizeSquared2D() <= FMath::Square(Radius);
	}
}