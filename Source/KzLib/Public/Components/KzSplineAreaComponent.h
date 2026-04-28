// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "KzSplineAreaComponent.generated.h"

/**
 * A Spline Component acting as a 2D polygonal boundary area.
 * Supports nesting other UKzSplineAreaComponent as holes (exclusion zones).
 */
UCLASS(ClassGroup = (KzLib), meta = (BlueprintSpawnableComponent))
class KZLIB_API UKzSplineAreaComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	UKzSplineAreaComponent(const FObjectInitializer& ObjectInitializer);

	/** Distance threshold used to adaptively sample the spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KzArea", meta = (ClampMin = "1.0"))
	float GenerationThreshold = 10.0f;

	/** Checks if a world point is inside this area and NOT inside any hole (XY projection only). */
	UFUNCTION(BlueprintPure, Category = "KzLib|Area")
	bool IsPointInside(const FVector& Point) const;

	/** Generates a random valid world point inside the area, respecting holes. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Area")
	FVector GetRandomPointInside(int32 MaxAttempts = 100) const;

	/**
	 * Returns the closest point on the 2D area to the given point.
	 * @param Point The target point to check against the area.
	 * @param bKeepInputZ If true, returns the Z coordinate from the input Point. If false, returns the component's Z.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Area", meta = (AdvancedDisplay = "bKeepInputZ"))
	FVector GetClosestPointOnArea(const FVector& Point, bool bKeepInputZ = true) const;

	/** Manually forces a rebuild of the cached polygon data and bounds. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Area")
	void CachePolygonData();

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	//~ End UActorComponent Interface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

protected:
	/** The cached polygon points in world space, generated adaptively. */
	UPROPERTY(Transient)
	TArray<FVector> CachedPolygon;

	/** The cached 2D bounding box for fast rejection and random generation. */
	UPROPERTY(Transient)
	FBox CachedBounds;
};