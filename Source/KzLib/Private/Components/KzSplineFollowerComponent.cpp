// Copyright 2026 kirzo

#include "Components/KzSplineFollowerComponent.h"
#include "Components/SplineComponent.h"
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
	Super::OnRegister();

	bTickInEditor = bShouldTickInEditor;
	InitializeReferences();
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

	SplineComponent = SplineComponentRef.GetComponent<USplineComponent>(this);
	if (!SplineComponent)
	{
		SplineComponent = Owner->FindComponentByClass<USplineComponent>();
	}

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
	InitializeReferences();
	Speed = FMath::Abs(Speed); // Ensure forward movement on reset
}

void UKzSplineFollowerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!SplineComponent || !UpdatedComponent || SplineLength <= 0.0f) return;

	// Clamp DeltaTime to avoid massive skips during hitches
	const float SafeDeltaTime = FMath::Min(DeltaTime, MaxTickTime);

	const float PrevDistance = CurrentDistance;
	const float DistanceDelta = Speed * SafeDeltaTime;
	float NewDistance = PrevDistance + DistanceDelta;

	// --- Handle Distance Wrap, Bounce, and Event Broadcasting ---
	if (SplineComponent->IsClosedLoop())
	{
		// Moving forward and crossed the finish line
		if (DistanceDelta > 0.0f && NewDistance >= SplineLength)
		{
			OnSplineLoopCompleted.Broadcast();
			NewDistance = FMath::Fmod(NewDistance, SplineLength);
		}
		// Moving backward and crossed the start line
		else if (DistanceDelta < 0.0f && NewDistance < 0.0f)
		{
			OnSplineLoopCompleted.Broadcast();
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
			OnDirectionChanged.Broadcast();
		}
		else if (NewDistance < 0.0f)
		{
			// Calculate how much it overshot backwards, and add it from the start
			NewDistance = FMath::Abs(NewDistance);
			Speed *= -1.0f;
			OnDirectionChanged.Broadcast();
		}
		NewDistance = FMath::Clamp(NewDistance, 0.0f, SplineLength);
	}
	else
	{
		// Non-looping boundaries
		if (DistanceDelta > 0.0f && PrevDistance < SplineLength && NewDistance >= SplineLength)
		{
			OnSplineEndReached.Broadcast();
		}
		else if (DistanceDelta < 0.0f && PrevDistance > 0.0f && NewDistance <= 0.0f)
		{
			OnSplineEndReached.Broadcast();
		}
		NewDistance = FMath::Clamp(NewDistance, 0.0f, SplineLength);
	}

	// Commit the safely calculated distance
	CurrentDistance = NewDistance;

	// --- Apply Movement ---
	const ETeleportType TeleportType = bTeleportPhysics ? ETeleportType::TeleportPhysics : ETeleportType::None;
	const FVector NewLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);

	// Save old location to calculate kinematic velocity
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	// Move the component
	if (bMatchRotation)
	{
		const FRotator NewRotation = SplineComponent->GetRotationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
		UpdatedComponent->SetWorldLocationAndRotation(NewLocation, NewRotation, false, nullptr, TeleportType);
	}
	else
	{
		UpdatedComponent->SetWorldLocation(NewLocation, false, nullptr, TeleportType);
	}

	// Inject kinematic velocity
	if (SafeDeltaTime > UE_KINDA_SMALL_NUMBER)
	{
		if (bTeleportPhysics)
		{
			// If we are teleporting, we explicitly kill the velocity to prevent physics glitches
			UpdatedComponent->ComponentVelocity = FVector::ZeroVector;
		}
		else
		{
			// Provide explicit linear velocity to the physics engine
			const FVector LinearVelocity = (NewLocation - OldLocation) / SafeDeltaTime;
			UpdatedComponent->ComponentVelocity = LinearVelocity;
		}
	}
}