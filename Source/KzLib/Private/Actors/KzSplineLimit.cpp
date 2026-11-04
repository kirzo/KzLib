// Copyright 2026 kirzo

#include "Actors/KzSplineLimit.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
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
#endif
}
