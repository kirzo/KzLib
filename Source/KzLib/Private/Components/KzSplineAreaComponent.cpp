// Copyright 2026 kirzo

#include "Components/KzSplineAreaComponent.h"
#include "Kismet/KzGeomLibrary.h"
#include "Math/Geometry/KzGeometry.h"

UKzSplineAreaComponent::UKzSplineAreaComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ClearSplinePoints(false);
	AddSplineLocalPoint(FVector(-500.0f, -500.0f, 0.0f));
	AddSplineLocalPoint(FVector(-500.0f, 500.0f, 0.0f));
	AddSplineLocalPoint(FVector(500.0f, 500.0f, 0.0f));
	AddSplineLocalPoint(FVector(500.0f, -500.0f, 0.0f));

	SetClosedLoop(true);

	CachedBounds = FBox(ForceInit);
}

void UKzSplineAreaComponent::BeginPlay()
{
	// Cache the geometry once at startup
	CachePolygonData();

	Super::BeginPlay();
}

FVector UKzSplineAreaComponent::GetClosestPointOnArea(const FVector& Point, bool bKeepInputZ) const
{
	// Determine the target Z coordinate based on the parameter
	const float TargetZ = bKeepInputZ ? Point.Z : GetComponentLocation().Z;

	// If the point is already inside the area, return its XY with the target Z
	if (IsPointInside(Point))
	{
		return FVector(Point.X, Point.Y, TargetZ);
	}

	// Safety check: ensure we have a valid polygon
	int32 NumPoints = CachedPolygon.Num();
	if (NumPoints < 3)
	{
		return FVector(Point.X, Point.Y, TargetZ);
	}

	// Flatten the target point for pure 2D distance calculation
	FVector Point2D(Point.X, Point.Y, 0.0f);

	FVector ClosestPoint2D = FVector::ZeroVector;
	float MinDistanceSquared = UE_MAX_FLT;

	// Iterate through all segments of the polygon perimeter
	for (int32 i = 0; i < NumPoints; ++i)
	{
		// Flatten segment start and end points
		FVector SegmentStart2D(CachedPolygon[i].X, CachedPolygon[i].Y, 0.0f);
		FVector SegmentEnd2D(CachedPolygon[(i + 1) % NumPoints].X, CachedPolygon[(i + 1) % NumPoints].Y, 0.0f);

		// Find the closest point on this specific segment
		FVector PointOnSegment2D = FMath::ClosestPointOnSegment(Point2D, SegmentStart2D, SegmentEnd2D);

		// Check if this point is closer than the previous ones
		float DistSquared = FVector::DistSquared(Point2D, PointOnSegment2D);
		if (DistSquared < MinDistanceSquared)
		{
			MinDistanceSquared = DistSquared;
			ClosestPoint2D = PointOnSegment2D;
		}
	}

	// Return the closest 2D position with the requested Z coordinate
	return FVector(ClosestPoint2D.X, ClosestPoint2D.Y, TargetZ);
}

void UKzSplineAreaComponent::CachePolygonData()
{
	CachedPolygon = UKzGeomLibrary::SplineToPolygon(this, true, GenerationThreshold);
	CachedPolygon = Kz::Geom::SimplifyPolygon(CachedPolygon, 2.0f);

	CachedBounds.Init();
	for (const FVector& Pt : CachedPolygon)
	{
		CachedBounds += Pt;
	}
}

bool UKzSplineAreaComponent::IsPointInside(const FVector& Point) const
{
	// Fast rejection via Axis-Aligned Bounding Box (XY only)
	if (Point.X < CachedBounds.Min.X || Point.X > CachedBounds.Max.X ||
		Point.Y < CachedBounds.Min.Y || Point.Y > CachedBounds.Max.Y)
	{
		return false;
	}

	return Kz::Geom::IsPointInPolygon2D(Point, CachedPolygon);
}

FVector UKzSplineAreaComponent::GetRandomPointInside(int32 MaxAttempts) const
{
	if (CachedPolygon.Num() < 3) return FVector::ZeroVector;

	for (int32 i = 0; i < MaxAttempts; ++i)
	{
		FVector RandomPoint = FMath::RandPointInBox(CachedBounds);
		if (IsPointInside(RandomPoint))
		{
			RandomPoint.Z = CachedPolygon[0].Z;
			return RandomPoint;
		}
	}
	return CachedPolygon[0];
}

#if WITH_EDITOR
void UKzSplineAreaComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Enforce closed loop constantly during editing
	if (IsClosedLoop() == false)
	{
		SetClosedLoop(true);
	}

	CachePolygonData();
}

bool UKzSplineAreaComponent::CanEditChange(const FProperty* InProperty) const
{
	if (InProperty->GetFName() == TEXT("bClosedLoop"))
	{
		return false;
	}

	return Super::CanEditChange(InProperty);
}
#endif