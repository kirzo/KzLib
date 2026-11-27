// Copyright 2026 kirzo

#include "Actors/KzInvisibleWall.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** How far around the wall a One Way wall starts watching a pawn, beyond the pawn's own size. */
	constexpr float NearbyMargin = 200.0f;

	/** The component a pawn moves with, the one that collides with the wall. */
	UPrimitiveComponent* GetMover(AActor* Actor)
	{
		const APawn* Pawn = Cast<APawn>(Actor);
		if (!Pawn)
		{
			return nullptr;
		}

		const UPawnMovementComponent* Movement = Pawn->GetMovementComponent();
		return Movement && Movement->UpdatedPrimitive ? Movement->UpdatedPrimitive.Get() : Cast<UPrimitiveComponent>(Pawn->GetRootComponent());
	}

	/** How far the mover reaches from its center along Direction. */
	double GetReach(const UPrimitiveComponent& Mover, const FVector& Direction)
	{
		if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(&Mover))
		{
			return Capsule->GetScaledCapsuleRadius();
		}

		const FVector Extent = Mover.Bounds.BoxExtent;
		return FMath::Abs(Direction.X) * Extent.X + FMath::Abs(Direction.Y) * Extent.Y + FMath::Abs(Direction.Z) * Extent.Z;
	}
}

AKzInvisibleWall::AKzInvisibleWall()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	WallComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Wall"));
	WallComponent->SetBoxExtent(FVector(25.0f, 250.0f, 200.0f));
	WallComponent->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	WallComponent->SetCollisionObjectType(ECC_WorldStatic);
	WallComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	WallComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	WallComponent->SetGenerateOverlapEvents(false);
	WallComponent->SetCanEverAffectNavigation(false);
	WallComponent->ShapeColor = FColor::Red;
	RootComponent = WallComponent;

	NearbyComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Nearby"));
	NearbyComponent->SetupAttachment(WallComponent);
	NearbyComponent->SetUsingAbsoluteScale(true);
	NearbyComponent->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	NearbyComponent->SetCollisionObjectType(ECC_WorldDynamic);
	NearbyComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	NearbyComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	NearbyComponent->SetCanEverAffectNavigation(false);
	NearbyComponent->SetVisibleFlag(false);

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/KzLib/EditorResources/InvisibleWall"));
		SpriteComponent->Sprite = SpriteTexture.Get();
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = TEXT("Volumes");
		SpriteComponent->SpriteInfo.DisplayName = NSLOCTEXT("KzSpriteCategory", "Volumes", "Volumes");
		SpriteComponent->SetUsingAbsoluteScale(true);
		SpriteComponent->SetupAttachment(WallComponent);
	}

	DirectionArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	if (DirectionArrow)
	{
		DirectionArrow->ArrowColor = FColor::Red;
		DirectionArrow->ArrowSize = 2.0f;
		DirectionArrow->SetupAttachment(WallComponent);
	}
#endif
}

void AKzInvisibleWall::SetEnabled_Implementation(bool bNewEnabled)
{
	bEnabled = bNewEnabled;
	Refresh();
}

void AKzInvisibleWall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	NearbyComponent->SetBoxExtent(WallComponent->GetScaledBoxExtent().GetAbs() + FVector(NearbyMargin));
	Refresh();
}

void AKzInvisibleWall::BeginPlay()
{
	Super::BeginPlay();

	NearbyComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleNearbyBegin);
	NearbyComponent->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleNearbyEnd);
	Refresh();
}

void AKzInvisibleWall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseNearby();
	Super::EndPlay(EndPlayReason);
}

void AKzInvisibleWall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (auto It = Nearby.CreateIterator(); It; ++It)
	{
		UPrimitiveComponent* Mover = It.Key().Get();
		if (!Mover)
		{
			It.RemoveCurrent();
			continue;
		}

		const bool bCrossing = IsCrossing(*Mover, It.Value());
		if (bCrossing != It.Value())
		{
			It.Value() = bCrossing;
			Mover->IgnoreComponentWhenMoving(WallComponent, bCrossing);
		}
	}
}

void AKzInvisibleWall::Refresh()
{
	const bool bWatch = bEnabled && bOneWay;
	WallComponent->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	NearbyComponent->SetCollisionEnabled(bWatch ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (!bWatch)
	{
		ReleaseNearby();
	}
	else if (HasActorBegunPlay() || IsActorBeginningPlay())
	{
		// Pawns already near as it turns on, or as play begins, raise no overlap of their own.
		NearbyComponent->UpdateOverlaps();
		TArray<UPrimitiveComponent*> Overlapping;
		NearbyComponent->GetOverlappingComponents(Overlapping);
		for (UPrimitiveComponent* Component : Overlapping)
		{
			Watch(Component);
		}
	}

#if WITH_EDITORONLY_DATA
	if (DirectionArrow)
	{
		DirectionArrow->SetVisibility(bOneWay);
	}
#endif
}

void AKzInvisibleWall::ReleaseNearby()
{
	for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, bool>& Pair : Nearby)
	{
		if (UPrimitiveComponent* Mover = Pair.Key.Get())
		{
			Mover->IgnoreComponentWhenMoving(WallComponent, false);
		}
	}
	Nearby.Reset();
	SetActorTickEnabled(false);
}

bool AKzInvisibleWall::IsCrossing(const UPrimitiveComponent& Mover, bool bWasCrossing) const
{
	// The arrow's way, flipped too when the wall is mirrored with a negative scale.
	const FVector Forward = WallComponent->GetComponentTransform().GetScaledAxis(EAxis::X).GetSafeNormal();
	const double Ahead = FVector::DotProduct(Mover.GetComponentLocation() - WallComponent->GetComponentLocation(), Forward);
	if (Ahead < 0.0)
	{
		return true;
	}

	// Through once its back clears the wall's front face.
	return bWasCrossing && Ahead < FMath::Abs(WallComponent->GetScaledBoxExtent().X) + GetReach(Mover, Forward);
}

void AKzInvisibleWall::HandleNearbyBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Watch(OtherComp);
}

void AKzInvisibleWall::Watch(UPrimitiveComponent* Component)
{
	UPrimitiveComponent* Mover = Component ? GetMover(Component->GetOwner()) : nullptr;
	if (!Mover || Mover != Component || Nearby.Contains(Mover))
	{
		return;
	}

	// One already in the wall as it turns on finishes crossing whichever way it goes.
	const bool bCrossing = IsCrossing(*Mover, true);
	Nearby.Add(Mover, bCrossing);
	Mover->IgnoreComponentWhenMoving(WallComponent, bCrossing);
	SetActorTickEnabled(true);
}

void AKzInvisibleWall::HandleNearbyEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherComp && Nearby.Remove(OtherComp) > 0)
	{
		OtherComp->IgnoreComponentWhenMoving(WallComponent, false);
		SetActorTickEnabled(!Nearby.IsEmpty());
	}
}