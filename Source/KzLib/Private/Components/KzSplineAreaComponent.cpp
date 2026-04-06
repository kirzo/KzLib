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