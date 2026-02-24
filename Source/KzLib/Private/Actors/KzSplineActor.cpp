// Copyright 2026 kirzo

#include "Actors/KzSplineActor.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"

AKzSplineActor::AKzSplineActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	RootComponent = SplineComponent;

	SetCanBeDamaged(false);
	bReplicates = false;
}

void AKzSplineActor::GroundSpline()
{
	if (!SplineComponent) return;

	UWorld* World = GetWorld();
	if (!World) return;

	SplineComponent->Modify();

	const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(GroundSpline), false, this);

	bool bUpdated = false;

	for (int32 i = 0; i < NumPoints; ++i)
	{
		const FVector PointLocation = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);

		// Start trace slightly above the point in case it is slightly clipped into the ground,
		// and trace far down (e.g., 100 meters) to find the floor.
		const FVector Start = PointLocation + FVector(0.0f, 0.0f, 100.0f);
		const FVector End = PointLocation - FVector(0.0f, 0.0f, 10000.0f);

		FHitResult Hit;
		// Use ECC_WorldStatic so the spline ignores dynamic objects, characters, or triggers
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, TraceParams))
		{
			// Update the point location without updating the whole spline on every iteration
			SplineComponent->SetLocationAtSplinePoint(i, Hit.Location, ESplineCoordinateSpace::World, false);
			bUpdated = true;
		}
	}

	if (bUpdated)
	{
		// Force the spline to recalculate tangents and bounds once all points are placed
		SplineComponent->UpdateSpline();
	}
}