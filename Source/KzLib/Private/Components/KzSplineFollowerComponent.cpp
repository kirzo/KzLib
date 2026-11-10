// Copyright 2026 kirzo

#include "Components/KzSplineFollowerComponent.h"
#include "Actors/KzSplineLimit.h"
#include "Actors/KzSplineMarker.h"
#include "Components/SplineComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "Core/KzTypes.h"
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

	// Markers project the way this follower does: once in game, every move in the editor, where the spline and markers may have just been edited.
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (Registry)
	{
		const bool bEditorWorld = !GetWorld()->IsGameWorld();
		Registry->ForEach<AKzSplineMarker>(SplineComponent.Get(), [this, bEditorWorld](AKzSplineMarker* Marker)
		{
			if (bEditorWorld || Marker->GetProjectionAxes() != IgnoredAxes)
			{
				Marker->UpdateProjection(IgnoredAxes);
			}
		});
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
	return FindClosestDistance(SplineComponent, Location, IgnoredAxes, bTeleported ? -1.0f : CurrentDistance);
}

namespace
{
	/** 1 on the axes that count, 0 on the ignored ones. */
	FVector KeptAxes(int32 IgnoredAxes)
	{
		return FVector(
			IgnoredAxes & static_cast<int32>(EKzAxis3D::X) ? 0.0 : 1.0,
			IgnoredAxes & static_cast<int32>(EKzAxis3D::Y) ? 0.0 : 1.0,
			IgnoredAxes & static_cast<int32>(EKzAxis3D::Z) ? 0.0 : 1.0);
	}

	/** Squared distance from the spline at Key to Location, on the kept axes only. */
	double MaskedDistSquared(const USplineComponent* Spline, float Key, const FVector& Location, const FVector& Kept)
	{
		return ((Spline->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World) - Location) * Kept).SizeSquared();
	}

	/**
	 * The engine only measures in 3D: pick the best of a few samples, then refine it with Newton steps on the masked distance.
	 * The steps land on the exact point whatever the segment's length, so a slowly moving target gives a smoothly moving result.
	 */
	float FindKeyOnSegment(const USplineComponent* Spline, const FVector& Location, int32 Segment, const FVector& Kept)
	{
		if (Kept == FVector::OneVector)
		{
			return Spline->FindInputKeyOnSegmentClosestToWorldLocation(Location, Segment);
		}

		constexpr int32 Samples = 8;
		float BestT = 0.0f;
		double BestDistSq = TNumericLimits<double>::Max();
		for (int32 Sample = 0; Sample <= Samples; ++Sample)
		{
			const float T = static_cast<float>(Sample) / Samples;
			const double DistSq = MaskedDistSquared(Spline, Segment + T, Location, Kept);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestT = T;
			}
		}

		// Kept within the best sample's neighbourhood, so a step can't jump to another dip of the curve.
		const float Low = FMath::Max(BestT - 1.0f / Samples, 0.0f);
		const float High = FMath::Min(BestT + 1.0f / Samples, 1.0f);

		float T = BestT;
		for (int32 Iteration = 0; Iteration < 10; ++Iteration)
		{
			const float Key = Segment + T;
			const FVector Delta = (Spline->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World) - Location) * Kept;
			const FVector Tangent = Spline->GetTangentAtSplineInputKey(Key, ESplineCoordinateSpace::World) * Kept;

			const double TangentSq = Tangent.SizeSquared();
			if (TangentSq < UE_SMALL_NUMBER) break;

			const float Step = static_cast<float>(FVector::DotProduct(Delta, Tangent) / TangentSq);
			T = FMath::Clamp(T - Step, Low, High);
			if (FMath::Abs(Step) < 1e-6f) break;
		}

		return Segment + T;
	}
}

float UKzSplineFollowerComponent::FindClosestDistance(const USplineComponent* Spline, const FVector& Location, int32 IgnoredAxes, float NearDistance)
{
	const int32 NumSegments = Spline ? Spline->GetNumberOfSplineSegments() : 0;
	if (NumSegments <= 0) return 0.0f;

	if (NearDistance < 0.0f && IgnoredAxes == 0)
	{
		return Spline->GetDistanceAlongSplineAtSplineInputKey(Spline->FindInputKeyClosestToWorldLocation(Location));
	}

	// Near a distance, only its segment and the neighbours, so a spline that doubles back can't steal the projection.
	int32 First = 0;
	int32 Last = NumSegments - 1;
	if (NearDistance >= 0.0f)
	{
		const int32 Segment = FMath::FloorToInt(Spline->GetInputKeyValueAtDistanceAlongSpline(NearDistance));
		First = Segment - 1;
		Last = Segment + 1;
	}

	const FVector Kept = KeptAxes(IgnoredAxes);
	float BestKey = 0.0f;
	double BestDistSq = TNumericLimits<double>::Max();
	for (int32 Candidate = First; Candidate <= Last; ++Candidate)
	{
		int32 Segment = Candidate;
		if (Spline->IsClosedLoop())
		{
			Segment = (Segment % NumSegments + NumSegments) % NumSegments;
		}
		else if (Segment < 0 || Segment >= NumSegments)
		{
			continue;
		}

		const float Key = FindKeyOnSegment(Spline, Location, Segment, Kept);
		const double DistSq = MaskedDistSquared(Spline, Key, Location, Kept);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestKey = Key;
		}
	}

	return Spline->GetDistanceAlongSplineAtSplineInputKey(BestKey);
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
