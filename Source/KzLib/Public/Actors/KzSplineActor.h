// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KzSplineActor.generated.h"

class UBillboardComponent;
class USplineComponent;

/**
 * A lightweight utility Actor that contains only a Spline Component.
 * Perfect for dropping into a level to define paths for KzSplineFollowerComponent, cameras, or AI.
 */
UCLASS(HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, Rendering))
class KZLIB_API AKzSplineActor : public AActor
{
	GENERATED_BODY()

public:

	AKzSplineActor();

	/** The main spline component used for path definition. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline")
	TObjectPtr<USplineComponent> SplineComponent;

#if WITH_EDITORONLY_DATA
	/** Editor-only billboard at the spline origin, so the actor is easy to spot and pick in the viewport. */
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
#endif

	/** Projects all spline points downwards and snaps them to the nearest static geometry. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spline|Utilities")
	void GroundSpline();
};