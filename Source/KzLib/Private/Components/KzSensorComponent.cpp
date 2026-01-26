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

void UKzSensorComponent::PerformScan()
{
	TArray<FKzSensorCandidate> Candidates;

	// 1. Fetch candidates from Registry (Preferred path)
	// This ensures we know exactly which Logic Object corresponds to which Shape
	if (AutoRegisterCategory && GetWorld())
	{
		if (UKzRegistrySubsystem* RegSub = GetWorld()->GetSubsystem<UKzRegistrySubsystem>())
		{
			TArray<UObject*> RawObjects = RegSub->GetItems<UObject>(AutoRegisterCategory);

			for (UObject* Obj : RawObjects)
			{
				if (!Obj) continue;

				UKzShapeComponent* FoundShape = nullptr;

				// Case A: The logic object IS the shape
				if (UKzShapeComponent* ShapeComp = Cast<UKzShapeComponent>(Obj))
				{
					FoundShape = ShapeComp;
				}
				// Case B: The logic object is an Actor
				else if (AActor* Actor = Cast<AActor>(Obj))
				{
					FoundShape = Actor->FindComponentByClass<UKzShapeComponent>();
				}
				// Case C: The logic object is a Component (e.g. InteractableComponent)
				else if (UActorComponent* Comp = Cast<UActorComponent>(Obj))
				{
					if (AActor* Owner = Comp->GetOwner())
					{
						FoundShape = Owner->FindComponentByClass<UKzShapeComponent>();
					}
				}

				// If we found a valid shape, we link it directly to the Logic Object
				if (FoundShape)
				{
					Candidates.Add({ FoundShape, Obj });
				}
			}
		}
	}

	TArray<UObject*> NewOverlaps;

	// 3. Iterate and Check GJK Intersection
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
				if (!NewOverlaps.Contains(HitObject))
				{
					NewOverlaps.Add(HitObject);
				}
			}
		}
	}

	// 4. Process Begin Overlaps (Logic Object Level)
	for (UObject* NewObj : NewOverlaps)
	{
		bool bIsNew = true;
		for (const TWeakObjectPtr<UObject>& Existing : CachedLogicObjects)
		{
			if (Existing.Get() == NewObj)
			{
				bIsNew = false;
				break;
			}
		}

		if (bIsNew)
		{
			CachedLogicObjects.Add(NewObj);
			OnObjectBeginOverlap.Broadcast(NewObj);
		}
	}

	// 5. Process End Overlaps (Logic Object Level)
	// Iterate backwards to safely remove
	for (int32 i = CachedLogicObjects.Num() - 1; i >= 0; --i)
	{
		UObject* OldObj = CachedLogicObjects[i].Get();

		// If object is invalid (GC'd) or not in the new list, it ended overlap
		if (!OldObj || !NewOverlaps.Contains(OldObj))
		{
			CachedLogicObjects.RemoveAtSwap(i);

			if (OldObj) // Only broadcast if it was valid logic object
			{
				OnObjectEndOverlap.Broadcast(OldObj);
			}
		}
	}
}