// Copyright 2026 kirzo

#include "Actors/KzStandIn.h"
#include "Camera/CameraComponent.h"
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
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CylinderMaterial = ShapeMaterial.Object;

	Cylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cylinder"));
	Cylinder->SetupAttachment(RootComponent);
	Cylinder->SetStaticMesh(CylinderMesh.Object);
	Cylinder->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Cylinder->SetCanEverAffectNavigation(false);

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);
	SkeletalMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SkeletalMesh->SetCanEverAffectNavigation(false);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(RootComponent);
	StaticMesh->SetMobility(EComponentMobility::Movable);
	StaticMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMesh->SetCanEverAffectNavigation(false);

	// Placed by the view set on it, wherever that is, and with nothing of its own to draw.
	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(RootComponent);
	PreviewCamera->SetUsingAbsoluteLocation(true);
	PreviewCamera->SetUsingAbsoluteRotation(true);
#if WITH_EDITORONLY_DATA
	PreviewCamera->bDrawFrustumAllowed = false;
	PreviewCamera->SetCameraMesh(nullptr);
#endif
}

FVector AKzStandIn::GetTargetLocation(AActor* RequestedBy) const
{
	return GetActorLocation() + GetActorUpVector() * (Height * 0.5f);
}

void AKzStandIn::SetPreviewView(const FMinimalViewInfo& View)
{
	PreviewCamera->SetWorldLocationAndRotation(View.Location, View.Rotation);
	PreviewCamera->SetFieldOfView(View.FOV);
	PreviewCamera->SetAspectRatio(View.AspectRatio);
	PreviewCamera->SetConstraintAspectRatio(View.bConstrainAspectRatio);
	PreviewCamera->PostProcessSettings = View.PostProcessSettings;
	PreviewCamera->SetPostProcessBlendWeight(View.PostProcessBlendWeight);
}

void AKzStandIn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateLook();
}

void AKzStandIn::UpdateLook()
{
	USkeletalMesh* SkeletalAsset = Cast<USkeletalMesh>(Mesh);
	SkeletalMesh->SetSkeletalMeshAsset(SkeletalAsset);
	SkeletalMesh->SetVisibility(SkeletalAsset != nullptr);

	UStaticMesh* StaticAsset = Cast<UStaticMesh>(Mesh);
	StaticMesh->SetStaticMesh(StaticAsset);
	StaticMesh->SetVisibility(StaticAsset != nullptr);

	// Without a mesh, the cylinder: standing on the actor's feet, at its size.
	Cylinder->SetVisibility(Mesh == nullptr);
	Cylinder->SetRelativeLocation(FVector(0.0, 0.0, Height * 0.5f));
	Cylinder->SetRelativeScale3D(FVector(Radius / CylinderRadius, Radius / CylinderRadius, Height / CylinderHeight));

	// The red instance isn't saved, so it is made again from the shape material after every load.
	UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Cylinder->GetMaterial(0));
	if (!Material && CylinderMaterial)
	{
		Material = UMaterialInstanceDynamic::Create(CylinderMaterial, this);
		Material->SetFlags(RF_Transient);
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.8f, 0.05f, 0.05f));
		Cylinder->SetMaterial(0, Material);
	}
}

void AKzStandIn::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	UpdateLook();

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