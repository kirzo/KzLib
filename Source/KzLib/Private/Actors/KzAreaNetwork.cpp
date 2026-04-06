// Copyright 2026 kirzo

#include "Actors/KzAreaNetwork.h"
#include "Components/KzSplineAreaComponent.h"

AKzAreaNetwork::AKzAreaNetwork(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	BaseArea = CreateDefaultSubobject<UKzSplineAreaComponent>(TEXT("BaseArea"));
	RootComponent = BaseArea;
}

bool AKzAreaNetwork::IsPointInside(const FVector& Point) const
{
	bool bInsideAdditive = false;

	// Check the base area first
	if (BaseArea->IsPointInside(Point))
	{
		bInsideAdditive = true;
	}
	else
	{
		// If not in base, check other additive modifiers
		for (const FKzAreaModifier& Mod : AreaModifiers)
		{
			if (Mod.Operation == EKzAreaOperation::Add)
			{
				if (UKzSplineAreaComponent* Comp = Mod.AreaReference.GetComponent<UKzSplineAreaComponent>(this))
				{
					if (Comp->IsPointInside(Point))
					{
						bInsideAdditive = true;
						break; // Found an additive area, no need to check more
					}
				}
			}
		}
	}

	// Early exit if it's not even inside any additive shape
	if (!bInsideAdditive)
	{
		return false;
	}

	// Check holes
	for (const FKzAreaModifier& Mod : AreaModifiers)
	{
		if (Mod.Operation == EKzAreaOperation::Subtract)
		{
			if (UKzSplineAreaComponent* Comp = Mod.AreaReference.GetComponent<UKzSplineAreaComponent>(this))
			{
				if (Comp->IsPointInside(Point))
				{
					return false;
				}
			}
		}
	}

	// It's in an additive shape and NOT in a hole. It's valid!
	return true;
}

FVector AKzAreaNetwork::GetRandomPointInside(int32 MaxAttempts) const
{
	// Collect all valid additive components to randomly sample from
	TArray<UKzSplineAreaComponent*> AdditiveAreas;
	AdditiveAreas.Add(BaseArea);

	for (const FKzAreaModifier& Mod : AreaModifiers)
	{
		if (Mod.Operation == EKzAreaOperation::Add)
		{
			if (UKzSplineAreaComponent* Comp = Mod.AreaReference.GetComponent<UKzSplineAreaComponent>(this))
			{
				AdditiveAreas.Add(Comp);
			}
		}
	}

	// Rejection Sampling using the full Network logic
	for (int32 i = 0; i < MaxAttempts; ++i)
	{
		// Pick a random additive area
		int32 RandomIndex = FMath::RandRange(0, AdditiveAreas.Num() - 1);

		// Ask that specific component for a raw random point
		FVector CandidatePoint = AdditiveAreas[RandomIndex]->GetRandomPointInside(5);

		// Validate against the entire network (which checks all subtractions)
		if (IsPointInside(CandidatePoint))
		{
			return CandidatePoint;
		}
	}

	// Fallback if the space is too complex or MaxAttempts ran out
	return BaseArea->GetComponentLocation();
}