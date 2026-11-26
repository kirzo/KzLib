// Copyright 2026 kirzo

#include "Actors/KzSplineLimit.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

AKzSplineLimit::AKzSplineLimit()
{
#if WITH_EDITORONLY_DATA
	if (SpriteComponent)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/KzLib/EditorResources/SplineLimit"));
		SpriteComponent->Sprite = SpriteTexture.Get();
	}

	if (ProjectionArrow)
	{
		ProjectionArrow->ArrowColor = FColor::Red;
	}

	DirectionArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	if (DirectionArrow)
	{
		DirectionArrow->ArrowColor = FColor::Red;
		DirectionArrow->ArrowSize = 2.0f;
		DirectionArrow->SetupAttachment(RootComponent);
	}
#endif
}

#if WITH_EDITORONLY_DATA
void AKzSplineLimit::UpdateArrows(const USplineComponent& SplineComponent)
{
	Super::UpdateArrows(SplineComponent);

	if (DirectionArrow)
	{
		const bool bOneWay = BlockDirection != EKzSplineLimitDirection::Both;
		DirectionArrow->SetVisibility(bOneWay);
		if (bOneWay)
		{
			const FVector Through = SplineComponent.GetDirectionAtDistanceAlongSpline(GetDistance(), ESplineCoordinateSpace::World) * (BlockDirection == EKzSplineLimitDirection::Backward ? 1.0 : -1.0);
			DirectionArrow->SetWorldLocationAndRotation(SplineComponent.GetLocationAtDistanceAlongSpline(GetDistance(), ESplineCoordinateSpace::World), Through.Rotation());
		}
	}
}
#endif
