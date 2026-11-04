// Copyright 2026 kirzo

#include "Actors/KzAlphaVolume.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/KzShapeComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Texture2D.h"
#include "Math/Geometry/Shapes/KzBox.h"
#include "UObject/ConstructorHelpers.h"

AKzAlphaVolume::AKzAlphaVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	ShapeComponent = CreateDefaultSubobject<UKzShapeComponent>(TEXT("Shape"));
	ShapeComponent->Shape.InitializeAs<FKzBox>(FVector(200.0f));
	ShapeComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	RootComponent = ShapeComponent;

	SetCanBeDamaged(false);
	bReplicates = false;

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/KzLib/EditorResources/AlphaVolume"));
		SpriteComponent->Sprite = SpriteTexture.Get();
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = TEXT("Volumes");
		SpriteComponent->SpriteInfo.DisplayName = NSLOCTEXT("KzSpriteCategory", "Volumes", "Volumes");
		SpriteComponent->SetupAttachment(ShapeComponent);
	}

	AxisArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("AxisArrow"));
	if (AxisArrow)
	{
		AxisArrow->ArrowColor = FColor::Yellow;
		AxisArrow->ArrowSize = 2.0f;
		AxisArrow->SetUsingAbsoluteScale(true);
		AxisArrow->SetupAttachment(ShapeComponent);
	}
#endif
}

bool AKzAlphaVolume::GetAxisExtent(FVector& OutDirection, double& OutMin, double& OutMax) const
{
	OutDirection = Axis.GetSafeNormal();
	if (!ShapeComponent || !ShapeComponent->Shape.IsValid() || OutDirection.IsZero()) return false;

	// Entry and exit faces along Axis, from the shape's support points.
	const FKzShapeInstance Shape = ShapeComponent->Shape * ShapeComponent->GetComponentScale();
	const FQuat Rotation = ShapeComponent->GetComponentQuat();
	const FVector LocalDirection = Rotation.UnrotateVector(OutDirection);
	OutMax = FVector::DotProduct(Rotation.RotateVector(Shape.GetSupportPoint(LocalDirection)), OutDirection);
	OutMin = FVector::DotProduct(Rotation.RotateVector(Shape.GetSupportPoint(-LocalDirection)), OutDirection);
	return true;
}

float AKzAlphaVolume::GetAlpha(const FVector& Location) const
{
	if (!ShapeComponent || !ShapeComponent->Shape.IsValid()) return 0.0f;

	const FKzShapeInstance Shape = ShapeComponent->Shape * ShapeComponent->GetComponentScale();
	const FVector Center = ShapeComponent->GetComponentLocation();
	const FQuat Rotation = ShapeComponent->GetComponentQuat();
	const bool bInside = Shape.IntersectsPoint(Center, Rotation, Location);

	FVector Direction;
	double Min = 0.0;
	double Max = 0.0;
	if (!UsesAxis() || !GetAxisExtent(Direction, Min, Max))
	{
		return bInside ? 1.0f : 0.0f;
	}

	const double Position = FVector::DotProduct(Location - Center, Direction);

	// Past the exit face counts only within the extrusion of the volume's middle section.
	if (!bInside && !(Position > Max && Shape.IntersectsPoint(Center, Rotation, Location - Direction * Position)))
	{
		return 0.0f;
	}

	if (!bProgressive)
	{
		return 1.0f;
	}

	const float T = Max > Min ? static_cast<float>(FMath::Clamp((Position - Min) / (Max - Min), 0.0, 1.0)) : 1.0f;
	const FRichCurve* RichCurve = Curve.GetRichCurveConst();
	return RichCurve && RichCurve->GetNumKeys() > 0 ? RichCurve->Eval(T) : T;
}

void AKzAlphaVolume::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AKzAlphaVolume::UnregisterAllComponents(bool bForReregister)
{
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Unregister(this);
	}

	Super::UnregisterAllComponents(bForReregister);
}

void AKzAlphaVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITORONLY_DATA
	if (AxisArrow)
	{
		FVector Direction;
		double Min = 0.0;
		double Max = 0.0;
		const bool bShowArrow = UsesAxis() && GetAxisExtent(Direction, Min, Max);

		AxisArrow->SetVisibility(bShowArrow);
		if (bShowArrow)
		{
			AxisArrow->SetWorldLocationAndRotation(ShapeComponent->GetComponentLocation() + Direction * Min, Direction.Rotation());
			AxisArrow->SetArrowLength(static_cast<float>(Max - Min) / AxisArrow->ArrowSize);
		}
	}
#endif
}
