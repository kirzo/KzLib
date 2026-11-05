// Copyright 2026 kirzo

#include "Components/KzSplineFollowerComponent.h"
#include "Actors/KzSplineLimit.h"
#include "Actors/KzSplineMarker.h"
#include "Components/SplineComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

UKzSplineFollowerComponent::UKzSplineFollowerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetAutoActivate(false);

	SetIsReplicatedByDefault(true);
}

void UKzSplineFollowerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKzSplineFollowerComponent, Speed);
	DOREPLIFETIME(UKzSplineFollowerComponent, CurrentDistance);
}

void UKzSplineFollowerComponent::OnRegister()
{
	// A projecting follower tracks something that moves during the frame, so it goes after movement.
	if (Mode == EKzSplineFollowMode::Projection)
	{
		SetTickGroup(TG_PostPhysics);
	}

	Super::OnRegister();

	bTickInEditor = bShouldTickInEditor;
	InitializeReferences();

	// Markers placed in the editor look here for the splines in use.
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void UKzSplineFollowerComponent::OnUnregister()
{
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Unregister(this);
	}

	Super::OnUnregister();
}

void UKzSplineFollowerComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeReferences();
}

void UKzSplineFollowerComponent::InitializeReferences()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UpdatedComponent = TargetComponentRef.GetComponent(this);
	if (!UpdatedComponent)
	{
		UpdatedComponent = Owner->GetRootComponent();
	}

	const USplineComponent* OldSpline = SplineComponent;

	SplineComponent = SplineComponentRef.GetComponent<USplineComponent>(this);
	if (!SplineComponent)
	{
		SplineComponent = Owner->FindComponentByClass<USplineComponent>();
	}

	if (SplineComponent != OldSpline)
	{
		MarkerSides.Reset();
	}

	bPendingTeleport = true;

	if (SplineComponent)
	{
		SplineLength = SplineComponent->GetSplineLength();
		CurrentDistance = StartAlpha * SplineLength;
	}
	else if (HasBegunPlay())
	{
		// Only disable tick if we are in game and definitively failed to find a spline
		SetComponentTickEnabled(false);
	}
}

void UKzSplineFollowerComponent::Reset()
{
	const AActor* Owner = GetOwner();
	if (Owner && !Owner->HasAuthority()) return;

	InitializeReferences();
	Speed = FMath::Abs(Speed); // Ensure forward movement on reset

	OnReset.Broadcast();
}

void UKzSplineFollowerComponent::SetDistance(float NewDistance)
{
	const AActor* Owner = GetOwner();
	if (Owner && !Owner->HasAuthority()) return;

	if (!SplineComponent || !UpdatedComponent || SplineLength <= 0.0f) return;

	// Apply mode-specific rules
	if (SplineComponent->IsClosedLoop())
	{
		NewDistance = FMath::Fmod(NewDistance, SplineLength);
		if (NewDistance < 0.0f)
		{
			NewDistance += SplineLength;
		}
	}
	else
	{
		NewDistance = FMath::Clamp(NewDistance, 0.0f, SplineLength);
	}

	CurrentDistance = NewDistance;
	bPendingTeleport = true;
	MoveToDistance(CurrentDistance, 0.0f);
}

void UKzSplineFollowerComponent::SetAlpha(float NewAlpha)
{
	NewAlpha = FMath::Clamp(NewAlpha, 0.0f, 1.0f);
	SetDistance(NewAlpha * SplineLength);
}

FTransform UKzSplineFollowerComponent::GetTransformAtDistance(float Distance) const
{
	if (!SplineComponent)
	{
		return UpdatedComponent ? UpdatedComponent->GetComponentTransform() : FTransform::Identity;
	}

	const FVector SplineLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

	if (bMatchRotation)
	{
		const FQuat SplineRotation = SplineComponent->GetQuaternionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		return FTransform(SplineRotation * RotationOffset.Quaternion(), SplineLocation + SplineRotation.RotateVector(LocationOffset));
	}

	return FTransform(UpdatedComponent ? UpdatedComponent->GetComponentQuat() : FQuat::Identity, SplineLocation);
}

void UKzSplineFollowerComponent::MoveToDistance(float Distance, float DeltaTime)
{
	if (!SplineComponent || !UpdatedComponent) return;

	const FTransform Transform = GetTransformAtDistance(Distance);
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	const bool bTeleport = bTeleportPhysics || DeltaTime <= UE_KINDA_SMALL_NUMBER;
	const ETeleportType TeleportType = bTeleport ? ETeleportType::TeleportPhysics : ETeleportType::None;

	if (bMatchRotation)
	{
		UpdatedComponent->SetWorldLocationAndRotation(Transform.GetLocation(), Transform.GetRotation(), false, nullptr, TeleportType);
	}
	else
	{
		UpdatedComponent->SetWorldLocation(Transform.GetLocation(), false, nullptr, TeleportType);
	}

	// Kinematic velocity for physics; teleports kill it so they don't fling whatever rides the component.
	UpdatedComponent->ComponentVelocity = bTeleport ? FVector::ZeroVector : (Transform.GetLocation() - OldLocation) / DeltaTime;
}

AActor* UKzSplineFollowerComponent::GetProjectedActor() const
{
	const UWorld* World = GetWorld();
	if (bProjectPlayerPawn && World && World->IsGameWorld())
	{
		const APlayerController* PlayerController = World->GetFirstPlayerController();
		return PlayerController ? PlayerController->GetPawn() : nullptr;
	}

	return ProjectionTarget;
}

bool UKzSplineFollowerComponent::CanMove() const
{
	if (!SplineComponent || !UpdatedComponent || SplineLength <= 0.0f) return false;

	const AActor* Owner = GetOwner();
	return !Owner || Owner->HasAuthority();
}

bool UKzSplineFollowerComponent::BeginMove(bool bTeleport)
{
	bTeleported = bPendingTeleport || bTeleport;
	bPendingTeleport = false;

	// In the editor the spline and its markers may have just been edited.
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (Registry && !GetWorld()->IsGameWorld())
	{
		Registry->ForEach<AKzSplineMarker>(SplineComponent.Get(), [](AKzSplineMarker* Marker) { Marker->UpdateProjection(); });
	}

	// A sticky limit holds the follower until it is disabled; a teleport releases it.
	const AKzSplineLimit* Stuck = StuckLimit.Get();
	const bool bStuck = Stuck && Stuck->bEnabled && !bTeleported;
	if (!bStuck)
	{
		StuckLimit.Reset();
	}
	return bStuck;
}

bool UKzSplineFollowerComponent::FinishMove(float NewDistance, float DeltaTime, bool bStuck)
{
	const float LimitedDistance = bStuck ? CurrentDistance : ApplyLimits(CurrentDistance, NewDistance);

	CurrentDistance = LimitedDistance;
	MoveToDistance(CurrentDistance, bTeleported ? 0.0f : DeltaTime);

	return LimitedDistance != NewDistance;
}

void UKzSplineFollowerComponent::ProjectTo(const FVector& Location, bool bTeleport, float DeltaTime)
{
	if (!CanMove()) return;

	const bool bStuck = BeginMove(bTeleport);
	FinishMove(bStuck ? CurrentDistance : ProjectLocation(Location), FMath::Min(DeltaTime, MaxTickTime), bStuck);
	UpdateMarkers();
}

float UKzSplineFollowerComponent::ProjectLocation(const FVector& Location) const
{
	const int32 NumSegments = SplineComponent->GetNumberOfSplineSegments();
	if (NumSegments <= 0) return CurrentDistance;

	float Key = 0.0f;
	if (bTeleported)
	{
		Key = SplineComponent->FindInputKeyClosestToWorldLocation(Location);
	}
	else
	{
		// Only the current segment and its neighbours, so a spline that doubles back can't steal the projection.
		const int32 Segment = FMath::FloorToInt(SplineComponent->GetInputKeyValueAtDistanceAlongSpline(CurrentDistance));
		float BestDistSq = TNumericLimits<float>::Max();

		for (int32 Offset = -1; Offset <= 1; ++Offset)
		{
			int32 Index = Segment + Offset;
			if (SplineComponent->IsClosedLoop())
			{
				Index = (Index % NumSegments + NumSegments) % NumSegments;
			}
			else if (Index < 0 || Index >= NumSegments)
			{
				continue;
			}

			const float CandidateKey = SplineComponent->FindInputKeyOnSegmentClosestToWorldLocation(Location, Index);
			const float DistSq = FVector::DistSquared(SplineComponent->GetLocationAtSplineInputKey(CandidateKey, ESplineCoordinateSpace::World), Location);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Key = CandidateKey;
			}
		}
	}

	return SplineComponent->GetDistanceAlongSplineAtSplineInputKey(Key);
}

void UKzSplineFollowerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Mode == EKzSplineFollowMode::Driven || !CanMove()) return;

	// Clamp DeltaTime to avoid massive skips during hitches
	const float SafeDeltaTime = FMath::Min(DeltaTime, MaxTickTime);
	const float PrevDistance = CurrentDistance;

	// A projected actor that changes or jumps too far teleports the follower.
	const AActor* Target = Mode == EKzSplineFollowMode::Projection ? GetProjectedActor() : nullptr;
	bool bTargetTeleported = false;
	if (Target)
	{
		const FVector Location = Target->GetActorLocation();
		bTargetTeleported = Target != LastProjectedActor.Get() || (TeleportDistance > 0.0f && FVector::DistSquared(Location, LastProjectedLocation) > FMath::Square(TeleportDistance));
		LastProjectedActor = Target;
		LastProjectedLocation = Location;
	}

	const bool bStuck = BeginMove(bTargetTeleported);

	float NewDistance = PrevDistance;
	bool bLooped = false;
	bool bBounced = false;
	bool bReachedEnd = false;

	if (bStuck)
	{
		NewDistance = PrevDistance;
	}
	else if (Mode == EKzSplineFollowMode::Projection)
	{
		if (Target)
		{
			NewDistance = ProjectLocation(Target->GetActorLocation());
		}
	}
	else
	{
		const float DistanceDelta = Speed * SafeDeltaTime;
		NewDistance = PrevDistance + DistanceDelta;

		if (SplineComponent->IsClosedLoop())
		{
			// Crossed the finish line forwards, or the start line backwards
			if ((DistanceDelta > 0.0f && NewDistance >= SplineLength) || (DistanceDelta < 0.0f && NewDistance < 0.0f))
			{
				bLooped = true;
				NewDistance = FMath::Fmod(NewDistance, SplineLength);
				if (NewDistance < 0.0f)
				{
					NewDistance += SplineLength;
				}
			}
		}
		else if (bBackAndForthLoop)
		{
			if (NewDistance > SplineLength)
			{
				// Calculate how much it overshot, and subtract it from the end
				NewDistance = SplineLength - (NewDistance - SplineLength);
				Speed *= -1.0f;
				bBounced = true;
			}
			else if (NewDistance < 0.0f)
			{
				// Calculate how much it overshot backwards, and add it from the start
				NewDistance = FMath::Abs(NewDistance);
				Speed *= -1.0f;
				bBounced = true;
			}
			NewDistance = FMath::Clamp(NewDistance, 0.0f, SplineLength);
		}
		else
		{
			// Detected from the pre-clamp delta: at high speed one tick can jump from well short of the end to past it.
			bReachedEnd =
				(DistanceDelta > 0.0f && PrevDistance < SplineLength && NewDistance >= SplineLength) ||
				(DistanceDelta < 0.0f && PrevDistance > 0.0f && NewDistance <= 0.0f);

			NewDistance = FMath::Clamp(NewDistance, 0.0f, SplineLength);
		}
	}

	if (FinishMove(NewDistance, SafeDeltaTime, bStuck))
	{
		bReachedEnd = false;
	}

	// Broadcast after committing, so handlers reading GetCurrentDistance() see where the follower actually is.
	if (bLooped)
	{
		OnSplineLoopCompleted.Broadcast();
	}
	if (bBounced)
	{
		OnDirectionChanged.Broadcast();
	}
	if (bReachedEnd)
	{
		OnSplineEndReached.Broadcast();
	}

	UpdateMarkers();
}

float UKzSplineFollowerComponent::ApplyLimits(float PrevDistance, float NewDistance)
{
	const bool bGameWorld = GetWorld()->IsGameWorld();
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);

	if (!Registry || bTeleported || (!bGameWorld && !bApplyLimitsInEditor))
	{
		LastHitLimit.Reset();
		return NewDistance;
	}

	// The nearest enabled limit on each side of where the follower was.
	AKzSplineLimit* Below = nullptr;
	AKzSplineLimit* Above = nullptr;

	Registry->ForEach<AKzSplineLimit>(SplineComponent.Get(), [&](AKzSplineLimit* Limit)
	{
		if (!Limit->bEnabled) return;

		const float LimitDistance = Limit->GetDistance();
		if (LimitDistance >= PrevDistance && (!Above || LimitDistance < Above->GetDistance()))
		{
			Above = Limit;
		}
		if (LimitDistance <= PrevDistance && (!Below || LimitDistance > Below->GetDistance()))
		{
			Below = Limit;
		}
	});

	// Stops just short, so the follower stays on its side and can still move away.
	constexpr float Margin = 0.1f;
	AKzSplineLimit* Hit = nullptr;

	if (Above && NewDistance >= Above->GetDistance())
	{
		NewDistance = Above->GetDistance() - Margin;
		Hit = Above;
	}
	else if (Below && NewDistance <= Below->GetDistance())
	{
		NewDistance = Below->GetDistance() + Margin;
		Hit = Below;
	}

	if (Hit && Hit != LastHitLimit.Get())
	{
		if (bGameWorld && Hit->bSticky)
		{
			StuckLimit = Hit;
		}
		OnLimitReached.Broadcast(Hit);
	}

	LastHitLimit = Hit;
	return NewDistance;
}

void UKzSplineFollowerComponent::UpdateMarkers()
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (!Registry) return;

	struct FCrossing
	{
		AKzSplineMarker* Marker;
		bool bForward;
		bool bTeleport;
	};

	TArray<FCrossing, TInlineAllocator<8>> Crossings;
	TMap<TObjectKey<AKzSplineMarker>, bool> NewSides;

	Registry->ForEach<AKzSplineMarker>(SplineComponent.Get(), [&](AKzSplineMarker* Marker)
	{
		const bool bPassed = Marker->GetDistance() <= CurrentDistance;
		NewSides.Add(TObjectKey<AKzSplineMarker>(Marker), bPassed);

		// A marker first seen already behind (start, streaming, placed in the editor) was jumped over, not crossed.
		const bool* bWasPassed = MarkerSides.Find(Marker);
		if (bWasPassed ? *bWasPassed != bPassed : bPassed)
		{
			Crossings.Add({ Marker, bPassed, bTeleported || !bWasPassed });
		}
	});

	// A passed marker moved to another spline is crossed back, so whatever it started gets undone.
	// One that just unregistered (its level streamed out) is forgotten instead.
	for (const TPair<TObjectKey<AKzSplineMarker>, bool>& Side : MarkerSides)
	{
		AKzSplineMarker* Marker = Side.Key.ResolveObjectPtr();
		if (Side.Value && Marker && !NewSides.Contains(Side.Key) && Marker->GetSpline() != SplineComponent)
		{
			Crossings.Add({ Marker, false, true });
		}
	}

	MarkerSides = MoveTemp(NewSides);

	// Backward crossings first; within each direction, in the order the follower meets them.
	Crossings.Sort([](const FCrossing& A, const FCrossing& B)
	{
		if (A.bForward != B.bForward)
		{
			return !A.bForward;
		}
		return A.bForward ? A.Marker->GetDistance() < B.Marker->GetDistance() : A.Marker->GetDistance() > B.Marker->GetDistance();
	});

	for (const FCrossing& Crossing : Crossings)
	{
		Crossing.Marker->NotifyCrossed(this, Crossing.bForward, Crossing.bTeleport);
	}
}
