// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/KzComponentReference.h"
#include "KzSplineMarker.generated.h"

class UArrowComponent;
class UBillboardComponent;
class USplineComponent;
class UKzSplineFollowerComponent;

/**
 * A point placed in the level and projected onto a spline; followers on that spline tell it when they cross it.
 * Placed in the editor, it takes the nearest spline a follower uses. It must live in the same level as its spline.
 */
UCLASS(Blueprintable, HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, Rendering, HLOD))
class KZLIB_API AKzSplineMarker : public AActor
{
	GENERATED_BODY()

public:
	AKzSplineMarker();

	/** The spline this marker sits on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Marker", meta = (AllowedClasses = "/Script/Engine.SplineComponent"))
	FKzComponentReference Spline;

	UFUNCTION(BlueprintPure, Category = "Spline Marker")
	USplineComponent* GetSpline() const;

	/** Distance along the spline of this marker's projection. */
	UFUNCTION(BlueprintPure, Category = "Spline Marker")
	float GetDistance() const { return Distance; }

	/**
	 * Projects the marker onto its spline, leaving out IgnoredAxes (EKzAxis3D flags) as the followers on it do.
	 * Followers do it the first time they meet the marker, and every frame in the editor so spline edits show up.
	 */
	UFUNCTION(BlueprintCallable, Category = "Spline Marker")
	void UpdateProjection(UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/KzLib.EKzAxis3D")) int32 IgnoredAxes = 0);

	/** The axes the last projection left out. */
	int32 GetProjectionAxes() const { return ProjectionAxes; }

	/** Called by a follower that crossed the marker. Teleport means it jumped past (start, respawn, SetDistance) rather than moved past. */
	virtual void NotifyCrossed(UKzSplineFollowerComponent* Follower, bool bForward, bool bTeleport);

	virtual void PostRegisterAllComponents() override;
	virtual void UnregisterAllComponents(bool bForReregister = false) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

protected:
	/** Called in game when a follower crosses the marker. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Spline Marker", meta = (DisplayName = "On Crossed"))
	void K2_OnCrossed(UKzSplineFollowerComponent* Follower, bool bForward, bool bTeleport);

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;

	/** Points from the marker to its projection on the spline. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ProjectionArrow = nullptr;
#endif

private:
	float Distance = 0.0f;
	int32 ProjectionAxes = 0;

	void RegisterInRegistry();

#if WITH_EDITOR
	/** Fills Spline with the nearest spline a follower uses. */
	void AssignNearestSpline();
#endif
};
