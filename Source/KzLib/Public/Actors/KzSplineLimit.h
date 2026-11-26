// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Actors/KzSplineMarker.h"
#include "Misc/KzToggleable.h"
#include "KzSplineLimit.generated.h"

/** Which way a limit stops followers, along its spline from the first point to the last. */
UENUM(BlueprintType)
enum class EKzSplineLimitDirection : uint8
{
	Both,
	/** Stops followers moving toward the spline's end and lets them through toward its start. */
	Forward,
	/** Stops followers moving toward the spline's start and lets them through toward its end. */
	Backward
};

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

	/** Which way it stops followers. In the editor, an arrow on the spline points the way it lets them through. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Limit")
	EKzSplineLimitDirection BlockDirection = EKzSplineLimitDirection::Both;

	/** A follower it stops stays there, in both directions, until the limit is disabled. Never in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Limit")
	bool bSticky = false;

	virtual void SetEnabled_Implementation(bool bNewEnabled) override { bEnabled = bNewEnabled; }

protected:
#if WITH_EDITORONLY_DATA
	/** Lies on the spline and points the way it lets followers through. Hidden with Both. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> DirectionArrow = nullptr;

	virtual void UpdateArrows(const USplineComponent& SplineComponent) override;
#endif
};
