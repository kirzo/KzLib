// Copyright 2026 kirzo

#include "Components/KzSensorComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "Collision/KzGJK.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

// Internal struct to pair a physical shape with the logical object it represents
struct FKzSensorCandidate
{
	UKzShapeComponent* Shape;
	UObject* LogicObject;
};

UKzSensorComponent::UKzSensorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Sensors are usually triggers/volumes, so default to wireframe
	bDrawSolid = false;
}

void UKzSensorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastScan += DeltaTime;
	if (TimeSinceLastScan >= ScanInterval)
	{
		PerformScan();
		TimeSinceLastScan = 0.0f;
	}
}

UKzShapeComponent* UKzSensorComponent::GetShapeForObject(UObject* LogicObject) const
{
	if (!LogicObject) return nullptr;

	// Find the overlap info matching this object
	const FKzOverlapResult* Found = CachedOverlaps.FindByPredicate([LogicObject](const FKzOverlapResult& Item)
		{
			return Item.LogicObject.Get() == LogicObject;
		});

	return Found ? Found->PhysicalShape.Get() : nullptr;
}

void UKzSensorComponent::PerformScan()
{
	TArray<FKzSensorCandidate> Candidates;

	// 1. Fetch candidates from Registry
	if (AutoRegisterCategory && GetWorld())
	{
		if (UKzRegistrySubsystem* RegSub = GetWorld()->GetSubsystem<UKzRegistrySubsystem>())
		{
			TArray<UObject*> RawObjects = RegSub->GetItems<UObject>(AutoRegisterCategory);

			for (UObject* Obj : RawObjects)
			{
				if (!Obj) continue;

				// Case A: The logic object IS the shape
				if (UKzShapeComponent* ShapeComp = Cast<UKzShapeComponent>(Obj))
				{
					Candidates.Add({ ShapeComp, Obj });
				}
				// Case B & C: The logic object is an Actor or a Component
				else
				{
					AActor* TargetActor = nullptr;
					if (AActor* Actor = Cast<AActor>(Obj))
					{
						TargetActor = Actor;
					}
					else if (UActorComponent* Comp = Cast<UActorComponent>(Obj))
					{
						TargetActor = Comp->GetOwner();
					}

					if (TargetActor)
					{
						// Fetch all shape components instead of just the first one
						TArray<UKzShapeComponent*> Shapes;
						TargetActor->GetComponents<UKzShapeComponent>(Shapes);

						for (UKzShapeComponent* FoundShape : Shapes)
						{
							if (FoundShape)
							{
								Candidates.Add({ FoundShape, Obj });
							}
						}
					}
				}
			}
		}
	}

	TArray<FKzOverlapResult> NewOverlaps;

	// 2. Iterate and Check GJK Intersection
	for (const FKzSensorCandidate& Candidate : Candidates)
	{
		UKzShapeComponent* TargetShape = Candidate.Shape;

		if (!TargetShape || TargetShape == this) continue;

		// Perform GJK with Scale applied to the shapes
		bool bIntersect = Kz::GJK::Intersect(Shape * GetComponentScale(), GetComponentLocation(), GetComponentQuat(), TargetShape->Shape * TargetShape->GetComponentScale(), TargetShape->GetComponentLocation(), TargetShape->GetComponentQuat());

		if (bIntersect)
		{
			// Identify the object
			UObject* HitObject = Candidate.LogicObject;

			if (HitObject)
			{
				// Avoid duplicates in the NEW list (e.g. if multiple shapes map to same object)
				bool bAlreadyAdded = NewOverlaps.ContainsByPredicate([HitObject](const FKzOverlapResult& Item)
					{
						return Item.LogicObject.Get() == HitObject;
					});

				if (!bAlreadyAdded)
				{
					FKzOverlapResult NewResult;
					NewResult.LogicObject = HitObject;
					NewResult.PhysicalShape = TargetShape; // Capture the shape here!
					NewOverlaps.Add(NewResult);
				}
			}
		}
	}

	// 3. Process Begin Overlaps
	for (const FKzOverlapResult& NewResult : NewOverlaps)
	{
		UObject* NewObj = NewResult.LogicObject.Get();
		if (!NewObj) continue;

		bool bIsNew = true;
		for (const FKzOverlapResult& Existing : CachedOverlaps)
		{
			if (Existing.LogicObject.Get() == NewObj)
			{
				bIsNew = false;
				break;
			}
		}

		if (bIsNew)
		{
			OnObjectBeginOverlap.Broadcast(NewResult);
		}
	}

	// 4. Process End Overlaps (Logic Object Level)
	// Iterate backwards to safely remove
	TArray<FKzOverlapResult> OldCache = CachedOverlaps;

	// Replace cache with new frame data (updates PhysicalShape references if they changed positions/priorities)
	CachedOverlaps = NewOverlaps;

	for (const FKzOverlapResult& OldResult : OldCache)
	{
		UObject* OldObj = OldResult.LogicObject.Get();

		// Check if this object is present in the new list
		bool bStillOverlapping = CachedOverlaps.ContainsByPredicate([OldObj](const FKzOverlapResult& Item)
			{
				return Item.LogicObject.Get() == OldObj;
			});

		if (!bStillOverlapping && OldObj)
		{
			OnObjectEndOverlap.Broadcast(OldResult);
		}
	}
}