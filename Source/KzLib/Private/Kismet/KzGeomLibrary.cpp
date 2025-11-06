// Copyright 2025 kirzo

#include "Kismet/KzGeomLibrary.h"
#include "Geometry/KzShape.h"
#include "KismetTraceUtils.h"

#include "KzDrawDebugHelpers.h"

bool UKzGeomLibrary::RayIntersectsSphere(const FVector RayOrigin, const FVector RayDir, float RayLength, const FVector SphereCenter, float SphereRadius, bool& bStartInside, float& OutDistance, FVector& OutHitPoint)
{
	const FVector OC = RayOrigin - SphereCenter;
	const float a = FVector::DotProduct(RayDir, RayDir); // should be 1.0 if normalized
	const float b = 2.0f * FVector::DotProduct(OC, RayDir);
	const float c = FVector::DotProduct(OC, OC) - (SphereRadius * SphereRadius);
	const float Discriminant = b * b - 4.0f * a * c;

	if (Discriminant < 0.0f)
	{
		return false; // no intersection
	}

	// Calculate the two potential intersection distances.
	const float SqrtDisc = FMath::Sqrt(Discriminant);
	const float Inv2A = 0.5f / a;
	const float t1 = (-b - SqrtDisc) * Inv2A;
	const float t2 = (-b + SqrtDisc) * Inv2A;

	bStartInside = false;
	float HitDist = t1;

	if (t1 < 0.0f)
	{
		HitDist = t2;
		bStartInside = true;
		if (t2 < 0.0f)
		{
			OutHitPoint = RayOrigin;
			return false; // both negative
		}
	}

	if (RayLength > 0.0f && HitDist > RayLength)
	{
		return false; // beyond ray length
	}

	OutDistance = HitDist;
	OutHitPoint = RayOrigin + RayDir * HitDist;
	return true;
}

bool UKzGeomLibrary::RayIntersectsBox(const FVector RayOrigin, const FVector RayDir, float RayLength, const FVector Center, const FVector BoxHalfSize, const FQuat Orientation, bool& bStartInside, float& OutDistance, FVector& OutHitPoint)
{
	FVector RayOrigin_Local = RayOrigin - Center;
	FVector RayDir_Local = RayDir;

	const bool bIsIdentity = Orientation.IsIdentity();
	if (!bIsIdentity)
	{
		RayOrigin_Local = Orientation.UnrotateVector(RayOrigin_Local);
		RayDir_Local = Orientation.UnrotateVector(RayDir_Local);
	}

	const FVector Min = -BoxHalfSize;
	const FVector Max =  BoxHalfSize;

	// Check if ray starts inside the box
	bStartInside = (RayOrigin_Local.X >= Min.X && RayOrigin_Local.X <= Max.X) && (RayOrigin_Local.Y >= Min.Y && RayOrigin_Local.Y <= Max.Y) && (RayOrigin_Local.Z >= Min.Z && RayOrigin_Local.Z <= Max.Z);

	if (bStartInside)
	{
		OutDistance = 0.0f;
		OutHitPoint = RayOrigin;
		return true;
	}

	float tmin = 0.f;
	float tmax = (RayLength <= 0.f) ? UE_BIG_NUMBER : RayLength;

	for (int32 k = 0; k < 3; ++k)
	{
		const float D = RayDir_Local[k];
		const float O = RayOrigin_Local[k];

		if (FMath::IsNearlyZero(D, UE_KINDA_SMALL_NUMBER))
		{
			// Ray is parallel to the slab planes on this axis
			if (O < Min[k] || O > Max[k])
			{
				return false; // Outside the box in this axis
			}
			// Otherwise, the ray stays inside slab range; no change to tmin/tmax
			continue;
		}

		const float InvD = 1.0f / D;
		float t0 = (Min[k] - O) * InvD;
		float t1 = (Max[k] - O) * InvD;

		if (InvD < 0.0f)
		{
			Swap(t0, t1);
		}

		tmin = FMath::Max(tmin, t0);
		tmax = FMath::Min(tmax, t1);

		if (tmax < tmin)
		{
			return false;
		}
	}

	if (tmin > RayLength)
	{
		return false; // Beyond max ray distance
	}

	OutDistance = FMath::Max(tmin, 0.0f);

	const FVector LocalHit = RayOrigin_Local + RayDir_Local * OutDistance;
	OutHitPoint = Center + (bIsIdentity ? LocalHit : Orientation.RotateVector(LocalHit));

	return true;
}

bool UKzGeomLibrary::LineIntersectsSphere(const UObject* WorldContextObject, const FVector Start, const FVector End, const FVector SphereCenter, float SphereRadius, bool& bStartInside, float& OutDistance, FVector& OutHitPoint, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector Dir; float Len;
	(End - Start).ToDirectionAndLength(Dir, Len);

	const bool bHit = RayIntersectsSphere(Start, Dir, Len, SphereCenter, SphereRadius, bStartInside, OutDistance, OutHitPoint);

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	FHitResult Hit;
	Hit.bBlockingHit = bHit;
	Hit.ImpactPoint = OutHitPoint;
	DrawDebugLineTraceSingle(World, Start, End, DrawDebugType, bHit, Hit, TraceColor, TraceHitColor, DrawTime);
	DrawDebugSphere(World, SphereCenter, SphereRadius, 16, bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

bool UKzGeomLibrary::LineIntersectsBox(const UObject* WorldContextObject, const FVector Start, const FVector End, const FVector Center, const FVector HalfSize, const FRotator Orientation, bool& bStartInside, float& OutDistance, FVector& OutHitPoint, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector Dir; float Len;
	(End - Start).ToDirectionAndLength(Dir, Len);

	const bool bHit = RayIntersectsBox(Start, Dir, Len, Center, HalfSize, Orientation.Quaternion(), bStartInside, OutDistance, OutHitPoint);

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	FHitResult Hit;
	Hit.bBlockingHit = bHit;
	Hit.ImpactPoint = OutHitPoint;
	DrawDebugLineTraceSingle(World, Start, End, DrawDebugType, bHit, Hit, TraceColor, TraceHitColor, DrawTime);
	DrawDebugBox(World, Center, HalfSize, Orientation.Quaternion(), bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

void UKzGeomLibrary::DrawDebugShape(const UObject* WorldContextObject, const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, FLinearColor Color, float LifeTime, float Thickness)
{
#if ENABLE_DRAW_DEBUG
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		::DrawDebugShape(World, Position, Orientation.Quaternion(), Shape, Color.ToFColor(true), false, LifeTime, SDPG_World, Thickness);
	}
#endif
}

FBox UKzGeomLibrary::GetShapeAABB(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape)
{
	if (!Shape.IsValid())
		return {};

	switch (Shape.GetType())
	{
		case EKzShapeType::Sphere:
		{
			return GetSphereAABB(Position, Shape.As<FKzSphere>().Radius);
		}
		case EKzShapeType::Box:
		{
			return GetBoxAABB(Position, Shape.As<FKzBox>().HalfSize, Orientation);
		}
		case EKzShapeType::Capsule:
		{
			const FKzCapsule& Capsule = Shape.As<FKzCapsule>();
			return GetCapsuleAABB(Position, Capsule.Radius, Capsule.HalfHeight, Orientation);
		}
		case EKzShapeType::Cylinder:
		{
			const FKzCylinder& Cylinder = Shape.As<FKzCylinder>();
			return GetCylinderAABB(Position, Cylinder.Radius, Cylinder.HalfHeight, Orientation);
		}
		default: return {};
	}
}

FBox UKzGeomLibrary::K2_GetShapeAABB(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape)
{
	return GetShapeAABB(Position, Orientation.Quaternion(), Shape);
}

FVector UKzGeomLibrary::ClosestPointOnShape(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, const FVector& Point)
{
	if (!Shape.IsValid())
		return {};

	switch (Shape.GetType())
	{
		case EKzShapeType::Sphere:
		{
			return ClosestPointOnSphere(Position, Shape.As<FKzSphere>().Radius, Point);
		}
		case EKzShapeType::Box:
		{
			return ClosestPointOnBox(Position, Shape.As<FKzBox>().HalfSize, Orientation, Point);
		}
		case EKzShapeType::Capsule:
		{
			const FKzCapsule& Capsule = Shape.As<FKzCapsule>();
			return ClosestPointOnCapsule(Position, Capsule.Radius, Capsule.HalfHeight, Orientation, Point);
		}
		case EKzShapeType::Cylinder:
		{
			const FKzCylinder& Cylinder = Shape.As<FKzCylinder>();
			return ClosestPointOnCylinder(Position, Cylinder.Radius, Cylinder.HalfHeight, Orientation, Point);
		}
		default: return {};
	}
}

FVector UKzGeomLibrary::K2_ClosestPointOnShape(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, const FVector Point)
{
	return ClosestPointOnShape(Position, Orientation.Quaternion(), Shape, Point);
}

bool UKzGeomLibrary::ShapeIntersectsPoint(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, const FVector& Point)
{
	if (!Shape.IsValid())
		return false;

	switch (Shape.GetType())
	{
		case EKzShapeType::Sphere:
		{
			return SphereIntersectsPoint(Position, Shape.As<FKzSphere>().Radius, Point);
		}
		case EKzShapeType::Box:
		{
			return BoxIntersectsPoint(Position, Shape.As<FKzBox>().HalfSize, Orientation, Point);
		}
		case EKzShapeType::Capsule:
		{
			const FKzCapsule& Capsule = Shape.As<FKzCapsule>();
			return CapsuleIntersectsPoint(Position, Capsule.Radius, Capsule.HalfHeight, Orientation, Point);
		}
		case EKzShapeType::Cylinder:
		{
			const FKzCylinder& Cylinder = Shape.As<FKzCylinder>();
			return CylinderIntersectsPoint(Position, Cylinder.Radius, Cylinder.HalfHeight, Orientation, Point);
		}
		default: return {};
	}
}

bool UKzGeomLibrary::K2_ShapeIntersectsPoint(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, const FVector Point)
{
	return ShapeIntersectsPoint(Position, Orientation.Quaternion(), Shape, Point);
}

bool UKzGeomLibrary::ShapeIntersectsSphere(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, const FVector& SphereCenter, float SphereRadius)
{
	if (!Shape.IsValid())
		return false;

	switch (Shape.GetType())
	{
		case EKzShapeType::Sphere:
		{
			return SphereIntersectsSphere(Position, Shape.As<FKzSphere>().Radius, SphereCenter, SphereRadius);
		}
		case EKzShapeType::Box:
		{
			return BoxIntersectsSphere(Position, Shape.As<FKzBox>().HalfSize, Orientation, SphereCenter, SphereRadius);
		}
		case EKzShapeType::Capsule:
		{
			const FKzCapsule& Capsule = Shape.As<FKzCapsule>();
			return CapsuleIntersectsSphere(Position, Capsule.Radius, Capsule.HalfHeight, Orientation, SphereCenter, SphereRadius);
		}
		case EKzShapeType::Cylinder:
		{
			const FKzCylinder& Cylinder = Shape.As<FKzCylinder>();
			return CylinderIntersectsSphere(Position, Cylinder.Radius, Cylinder.HalfHeight, Orientation, SphereCenter, SphereRadius);
		}
		default: return {};
	}
}

bool UKzGeomLibrary::K2_ShapeIntersectsSphere(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, const FVector SphereCenter, float SphereRadius)
{
	return ShapeIntersectsSphere(Position, Orientation.Quaternion(), Shape, SphereCenter, SphereRadius);
}

FKzShapeInstance UKzGeomLibrary::MakeSphere(const float Radius)
{
	return FKzShapeInstance::Make<FKzSphere>(Radius);
}

FBox UKzGeomLibrary::GetSphereAABB(const FVector Center, float Radius)
{
	const FVector Extent(Radius);
	return FBox(Center - Extent, Center + Extent);
}

FVector UKzGeomLibrary::ClosestPointOnSphere(const FVector Center, float Radius, FVector Point)
{
	const FVector LocalPoint = Point - Center;
	const float VSq = LocalPoint.SizeSquared();
	if (VSq <= FMath::Square(Radius)) return Point;
	return Center + LocalPoint * (Radius * FMath::InvSqrt(VSq));
}

bool UKzGeomLibrary::SphereIntersectsPoint(const FVector Center, float Radius, const FVector Point)
{
	return FVector::DistSquared(Center, Point) <= FMath::Square(Radius);
}

bool UKzGeomLibrary::SphereIntersectsSphere(const FVector CenterA, float RadiusA, const FVector CenterB, float RadiusB)
{
	return FVector::DistSquared(CenterA, CenterB) <= FMath::Square(RadiusA + RadiusB);
}

FKzShapeInstance UKzGeomLibrary::MakeBox(const FVector HalfSize)
{
	return FKzShapeInstance::Make<FKzBox>(HalfSize);
}

FBox UKzGeomLibrary::GetBoxAABB(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation)
{
	const FVector AxisX = Orientation.GetAxisX();
	const FVector AxisY = Orientation.GetAxisY();
	const FVector AxisZ = Orientation.GetAxisZ();

	const FVector AbsX = AxisX.GetAbs();
	const FVector AbsY = AxisY.GetAbs();
	const FVector AbsZ = AxisZ.GetAbs();

	const FVector WorldHalfExtent(
		AbsX.X * HalfSize.X + AbsY.X * HalfSize.Y + AbsZ.X * HalfSize.Z,
		AbsX.Y * HalfSize.X + AbsY.Y * HalfSize.Y + AbsZ.Y * HalfSize.Z,
		AbsX.Z * HalfSize.X + AbsY.Z * HalfSize.Y + AbsZ.Z * HalfSize.Z
	);

	return FBox(Center - WorldHalfExtent, Center + WorldHalfExtent);
}

FBox UKzGeomLibrary::K2_GetBoxAABB(const FVector Center, const FVector HalfSize, const FRotator Orientation)
{
	return GetBoxAABB(Center, HalfSize, Orientation.Quaternion());
}

FVector UKzGeomLibrary::ClosestPointOnBox(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation, const FVector& Point)
{
	const FVector LocalPoint = Orientation.UnrotateVector(Point - Center);
	return Center + Orientation.RotateVector(LocalPoint.BoundToBox(-HalfSize, HalfSize));
}

FVector UKzGeomLibrary::K2_ClosestPointOnBox(const FVector Center, const FVector HalfSize, const FRotator Orientation, const FVector Point)
{
	return ClosestPointOnBox(Center, HalfSize, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::BoxIntersectsPoint(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation, const FVector& Point)
{
	const FVector LocalPoint = Orientation.UnrotateVector(Point - Center);
	return
		FMath::IsWithinInclusive(LocalPoint.X, -HalfSize.X, HalfSize.X) &&
		FMath::IsWithinInclusive(LocalPoint.Y, -HalfSize.Y, HalfSize.Y) &&
		FMath::IsWithinInclusive(LocalPoint.Z, -HalfSize.Z, HalfSize.Z);
}

bool UKzGeomLibrary::K2_BoxIntersectsPoint(const FVector Center, const FVector HalfSize, const FRotator Orientation, const FVector Point)
{
	return BoxIntersectsPoint(Center, HalfSize, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::BoxIntersectsSphere(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius)
{
	const FVector LocalCenter = Orientation.UnrotateVector(SphereCenter - Center);
	const FVector ClosestPoint = LocalCenter.BoundToBox(-HalfSize, HalfSize);

	const float DistSq = FVector::DistSquared(LocalCenter, ClosestPoint);
	return DistSq <= FMath::Square(SphereRadius);
}

bool UKzGeomLibrary::K2_BoxIntersectsSphere(const FVector Center, const FVector HalfSize, const FRotator Orientation, const FVector SphereCenter, float SphereRadius)
{
	return BoxIntersectsSphere(Center, HalfSize, Orientation.Quaternion(), SphereCenter, SphereRadius);
}

FKzShapeInstance UKzGeomLibrary::MakeCapsule(const float Radius, const float HalfHeight)
{
	return FKzShapeInstance::Make<FKzCapsule>(Radius, HalfHeight);
}

FBox UKzGeomLibrary::GetCapsuleAABB(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation)
{
	// Vector from center to one cap center (local +Z axis)
	const FVector Axis = Orientation.GetAxisZ();
	const FVector HalfSegment = Axis * (HalfHeight - Radius);

	// Endpoints of the capsule spine
	const FVector CapA = Center - HalfSegment;
	const FVector CapB = Center + HalfSegment;

	// Compute min/max bounds including the radius
	const FVector Min = FVector::Min(CapA, CapB) - FVector(Radius);
	const FVector Max = FVector::Max(CapA, CapB) + FVector(Radius);

	return FBox(Min, Max);
}

FBox UKzGeomLibrary::K2_GetCapsuleAABB(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation)
{
	return GetCapsuleAABB(Center, Radius, HalfHeight, Orientation.Quaternion());
}

FVector UKzGeomLibrary::ClosestPointOnCapsule(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point)
{
	const FVector LocalPoint = Orientation.UnrotateVector(Point - Center);
	if (FMath::Abs(LocalPoint.Z) <= HalfHeight - Radius)
	{
		return Center + Orientation.RotateVector(LocalPoint.GetClampedToMaxSize2D(Radius));
	}
	else
	{
		const FVector Offset = FVector::UpVector * FMath::Sign(LocalPoint.Z) * (HalfHeight - Radius);
		return Center + Orientation.RotateVector((LocalPoint - Offset).GetClampedToMaxSize(Radius) + Offset);
	}
}

FVector UKzGeomLibrary::K2_ClosestPointOnCapsule(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector Point)
{
	return ClosestPointOnCapsule(Center, Radius, HalfHeight, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::CapsuleIntersectsPoint(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point)
{
	const FVector LocalPoint = Orientation.UnrotateVector(Point - Center);

	return
		(FMath::Abs(LocalPoint.Z) <= HalfHeight - Radius &&
		 LocalPoint.SizeSquared2D() <= FMath::Square(Radius)) ||
		FVector::DistSquared(FVector::UpVector * (HalfHeight - Radius) * FMath::Sign(LocalPoint.Z), LocalPoint) <= FMath::Square(Radius);
}

bool UKzGeomLibrary::K2_CapsuleIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector Point)
{
	return CapsuleIntersectsPoint(Center, Radius, HalfHeight, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::CapsuleIntersectsSphere(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius)
{
	const FVector LocalCenter = Orientation.UnrotateVector(SphereCenter - Center);

	FVector ClosestPoint;
	if (FMath::Abs(LocalCenter.Z) <= HalfHeight - Radius)
	{
		ClosestPoint = LocalCenter.GetClampedToMaxSize2D(Radius);
	}
	else
	{
		const FVector Offset = FVector::UpVector * FMath::Sign(LocalCenter.Z) * (HalfHeight - Radius);
		ClosestPoint = (LocalCenter - Offset).GetClampedToMaxSize(Radius) + Offset;
	}

	const float DistSq = FVector::DistSquared(LocalCenter, ClosestPoint);
	return DistSq <= FMath::Square(SphereRadius);
}

bool UKzGeomLibrary::K2_CapsuleIntersectsSphere(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector SphereCenter, float SphereRadius)
{
	return CapsuleIntersectsSphere(Center, Radius, HalfHeight, Orientation.Quaternion(), SphereCenter, SphereRadius);
}

FKzShapeInstance UKzGeomLibrary::MakeCylinder(const float Radius, const float HalfHeight)
{
	return FKzShapeInstance::Make<FKzCylinder>(Radius, HalfHeight);
}

FBox UKzGeomLibrary::GetCylinderAABB(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation)
{
	// Axis of the cylinder in world space
	const FVector Axis = Orientation.GetAxisZ();

	// Offset to top/bottom faces
	const FVector HalfSegment = Axis * HalfHeight;

	// Compute the projected world-space extents
	const FVector AbsAxisZ(FMath::Abs(Axis.X), FMath::Abs(Axis.Y), FMath::Abs(Axis.Z));
	const FVector RadialExtent = FVector(Radius) * FVector(1.0f - AbsAxisZ.X, 1.0f - AbsAxisZ.Y, 1.0f - AbsAxisZ.Z);

	// Combine caps and radius
	const FVector Top = Center + HalfSegment;
	const FVector Bottom = Center - HalfSegment;

	const FVector Min = FVector::Min(Top, Bottom) - FVector(Radius);
	const FVector Max = FVector::Max(Top, Bottom) + FVector(Radius);

	return FBox(Min, Max);
}

FBox UKzGeomLibrary::K2_GetCylinderAABB(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation)
{
	return GetCylinderAABB(Center, Radius, HalfHeight, Orientation.Quaternion());
}

FVector UKzGeomLibrary::ClosestPointOnCylinder(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point)
{
	const FVector LocalPoint = Orientation.UnrotateVector(Point - Center);

	FVector ClosestPoint = LocalPoint.GetClampedToMaxSize2D(Radius);
	if (FMath::Abs(LocalPoint.Z) > HalfHeight)
	{
		ClosestPoint.Z = FMath::Clamp(LocalPoint.Z, -HalfHeight, HalfHeight);
	}

	return Center + Orientation.RotateVector(ClosestPoint);
}

FVector UKzGeomLibrary::K2_ClosestPointOnCylinder(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector Point)
{
	return ClosestPointOnCylinder(Center, Radius, HalfHeight, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::CylinderIntersectsPoint(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point)
{
	const FVector LocalPoint = Orientation.UnrotateVector(Point - Center);
	return FMath::Abs(LocalPoint.Z) <= HalfHeight && LocalPoint.SizeSquared2D() <= FMath::Square(Radius);
}

bool UKzGeomLibrary::K2_CylinderIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, FVector Point)
{
	return CylinderIntersectsPoint(Center, Radius, HalfHeight, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::CylinderIntersectsSphere(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius)
{
	const FVector LocalCenter = Orientation.UnrotateVector(SphereCenter - Center);

	FVector ClosestPoint = LocalCenter.GetClampedToMaxSize2D(Radius);
	if (FMath::Abs(LocalCenter.Z) > HalfHeight)
	{
		ClosestPoint.Z = FMath::Clamp(LocalCenter.Z, -HalfHeight, HalfHeight);
	}

	const float DistSq = FVector::DistSquared(LocalCenter, ClosestPoint);
	return DistSq <= FMath::Square(SphereRadius);
}

bool UKzGeomLibrary::K2_CylinderIntersectsSphere(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector SphereCenter, float SphereRadius)
{
	return CylinderIntersectsSphere(Center, Radius, HalfHeight, Orientation.Quaternion(), SphereCenter, SphereRadius);
}