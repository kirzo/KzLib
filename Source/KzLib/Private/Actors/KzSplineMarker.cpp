// Copyright 2026 kirzo

#include "Actors/KzSplineMarker.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/KzSplineFollowerComponent.h"
#include "Components/SplineComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AKzSplineMarker::AKzSplineMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	SetCanBeDamaged(false);
	bReplicates = false;

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/KzLib/EditorResources/SplineMarker"));
		SpriteComponent->Sprite = SpriteTexture.Get();
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = TEXT("Splines");
		SpriteComponent->SpriteInfo.DisplayName = NSLOCTEXT("KzSpriteCategory", "Splines", "Splines");
		SpriteComponent->SetupAttachment(RootComponent);
	}

	ProjectionArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ProjectionArrow"));
	if (ProjectionArrow)
	{
		ProjectionArrow->ArrowColor = FColor::Cyan;
		ProjectionArrow->SetupAttachment(RootComponent);
	}
#endif
}

USplineComponent* AKzSplineMarker::GetSpline() const
{
	return Spline.GetComponent<USplineComponent>(this);
}

void AKzSplineMarker::UpdateProjection()
{
	const USplineComponent* SplineComponent = GetSpline();
	if (!SplineComponent)
	{
		Distance = 0.0f;
		return;
	}

	const FVector Location = GetActorLocation();
	const float Key = SplineComponent->FindInputKeyClosestToWorldLocation(Location);
	Distance = SplineComponent->GetDistanceAlongSplineAtSplineInputKey(Key);

#if WITH_EDITORONLY_DATA
	if (ProjectionArrow)
	{
		const FVector ToProjection = SplineComponent->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World) - Location;
		ProjectionArrow->SetWorldRotation(ToProjection.IsNearlyZero() ? GetActorRotation() : ToProjection.Rotation());
		ProjectionArrow->SetArrowLength(ToProjection.Size());
	}
#endif
}

void AKzSplineMarker::NotifyCrossed(UKzSplineFollowerComponent* Follower, bool bForward, bool bTeleport)
{
	// Blueprint logic only runs in game, never on editor previews.
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		K2_OnCrossed(Follower, bForward, bTeleport);
	}
}

void AKzSplineMarker::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	RegisterInRegistry();
}

void AKzSplineMarker::UnregisterAllComponents(bool bForReregister)
{
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Unregister(this);
	}

	Super::UnregisterAllComponents(bForReregister);
}

void AKzSplineMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (World && !World->IsGameWorld() && !GetSpline())
	{
		AssignNearestSpline();
	}
#endif

	UpdateProjection();

	// The edit that rebuilt the marker may have changed its spline.
	RegisterInRegistry();
}

void AKzSplineMarker::BeginPlay()
{
	Super::BeginPlay();
	UpdateProjection();
}

void AKzSplineMarker::RegisterInRegistry()
{
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Register(this, GetSpline());
	}
}

#if WITH_EDITOR
void AKzSplineMarker::AssignNearestSpline()
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (!Registry) return;

	const FVector Location = GetActorLocation();
	USplineComponent* Nearest = nullptr;
	double NearestDistSq = TNumericLimits<double>::Max();

	Registry->ForEach<UKzSplineFollowerComponent>([&](UKzSplineFollowerComponent* Follower)
	{
		USplineComponent* Candidate = Follower->GetSpline();
		if (!Candidate) return;

		const double DistSq = FVector::DistSquared(Candidate->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World), Location);
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Candidate;
		}
	});

	if (Nearest)
	{
		AActor* SplineOwner = Nearest->GetOwner();
		Spline.OverrideActor = SplineOwner;
		Spline.ComponentName = Nearest == SplineOwner->GetRootComponent() ? NAME_None : Nearest->GetFName();
	}
}
#endif
