// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Actors/KzSplineMarker.h"
#include "Misc/KzToggleable.h"
#include "KzSplineLimit.generated.h"

/**
 * While enabled, followers on its spline can't go past it; two enabled limits close a segment.
 * Teleports (start, respawn, SetDistance) ignore limits.
 */
UCLASS(Blueprintable)
class KZLIB_API AKzSplineLimit : public AKzSplineMarker, public IKzToggleable
{
	GENERATED_BODY()

public:
	AKzSplineLimit();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Limit")
	bool bEnabled = false;

	/** A follower that reaches it stays there, in both directions, until the limit is disabled. Never in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Limit")
	bool bSticky = false;

	virtual void SetEnabled_Implementation(bool bNewEnabled) override { bEnabled = bNewEnabled; }
};
