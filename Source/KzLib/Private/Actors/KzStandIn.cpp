// Copyright 2026 kirzo

#include "Actors/KzStandIn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The engine's cylinder: 100 tall, 50 in radius, its pivot in the middle. */
	constexpr float CylinderHeight = 100.0f;
	constexpr float CylinderRadius = 50.0f;

	/** The steepest ground a character stands on by default, about 45 degrees. */
	constexpr double WalkableNormalZ = 0.71;

	/** Surfaces looked through on the way down before giving up. */
	constexpr int32 MaxSurfaces = 8;
}

AKzStandIn::AKzStandIn()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	bReplicates = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CylinderMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Cylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cylinder"));
	Cylinder->SetupAttachment(RootComponent);
	Cylinder->SetStaticMesh(CylinderMesh.Object);
	Cylinder->SetMaterial(0, CylinderMaterial.Object);
	Cylinder->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Cylinder->SetCanEverAffectNavigation(false);

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);
	SkeletalMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SkeletalMesh->SetCanEverAffectNavigation(false);
}

void AKzStandIn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SkeletalMesh->SetSkeletalMeshAsset(Mesh);
	SkeletalMesh->SetVisibility(Mesh != nullptr);

	// Standing on the actor's feet, at its size.
	Cylinder->SetVisibility(Mesh == nullptr);
	Cylinder->SetRelativeLocation(FVector(0.0, 0.0, Height * 0.5f));
	Cylinder->SetRelativeScale3D(FVector(Radius / CylinderRadius, Radius / CylinderRadius, Height / CylinderHeight));

	UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Cylinder->GetMaterial(0));
	if (!Material)
	{
		Material = Cylinder->CreateAndSetMaterialInstanceDynamic(0);
		if (Material)
		{
			Material->SetFlags(RF_Transient);
		}
	}
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.8f, 0.05f, 0.05f));
	}
}

void AKzStandIn::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AKzStandIn::UnregisterAllComponents(bool bForReregister)
{
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Unregister(this);
	}

	Super::UnregisterAllComponents(bForReregister);
}

void AKzStandIn::BeginPlay()
{
	Super::BeginPlay();

	// Gone in game, but neither HiddenInGame nor editor-only: the editor's game views, the camera previews among them, wouldn't draw it.
	Destroy();
}

#if WITH_EDITOR
void AKzStandIn::PostEditMove(bool bFinished)
{
	// While dragged too, so the cameras that preview it follow along.
	if (bSnapToGround)
	{
		SnapToGround();
	}

	Super::PostEditMove(bFinished);
}
#endif

void AKzStandIn::SnapToGround()
{
	FVector Ground;
	if (FindGround(Ground))
	{
		SetActorLocation(Ground);
	}
}

bool AKzStandIn::FindGround(FVector& OutLocation) const
{
	const UWorld* World = GetWorld();
	if (!World) return false;

	// A sphere as wide as the stand-in, down from Snap Above over its feet to Snap Below them. Each surface met is set aside and the sweep
	// goes on below it, so a bridge overhead or a ceiling doesn't hide the ground; the walkable one nearest the feet wins.
	const FVector Feet = GetActorLocation();
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	const FVector Start = Feet + FVector(0.0, 0.0, SnapAbove + Radius);
	const FVector End = Feet - FVector(0.0, 0.0, SnapBelow - Radius);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(KzStandIn), false, this);
	bool bFound = false;
	double BestGap = TNumericLimits<double>::Max();

	for (int32 Surface = 0; Surface < MaxSurfaces; ++Surface)
	{
		FHitResult Hit;
		if (!World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, SnapChannel, Sphere, Params)) break;

		const bool bGround = !Hit.bStartPenetrating && Hit.ImpactNormal.Z >= WalkableNormalZ && !Cast<APawn>(Hit.GetActor());
		if (bGround)
		{
			const FVector Standing(Feet.X, Feet.Y, Hit.Location.Z - Radius);
			const double Gap = FMath::Abs(Standing.Z - Feet.Z);
			if (Gap < BestGap)
			{
				BestGap = Gap;
				OutLocation = Standing;
				bFound = true;
			}
		}

		if (!Hit.GetComponent()) break;
		Params.AddIgnoredComponent(Hit.GetComponent());
	}

	return bFound;
}