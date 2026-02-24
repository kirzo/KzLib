// Copyright 2026 kirzo

#pragma once

#include "KzLibMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/KzComponentSocketReference.h"
#include "KzSplineFollowerComponent.generated.h"

class USplineComponent;

/**
 * Component that automatically moves a specified SceneComponent along a SplineComponent.
 */
UCLASS(ClassGroup = ("Movement"), meta = (BlueprintSpawnableComponent))
class KZLIB_API UKzSplineFollowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** The component that will be moved along the spline. If empty, the Owner's RootComponent will be used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (NoSocket, NoOffset))
	FKzComponentSocketReference TargetComponentRef;

	/** The Spline to follow. If empty, it will search for the first USplineComponent in the Owner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (NoSocket, NoOffset, AllowedClasses = "/Script/Engine.SplineComponent"))
	FKzComponentSocketReference SplineComponentRef;

	/** Normalized starting position on the spline (0.0 to 1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (ClampMin = "0", ClampMax = "1"))
	float StartAlpha = 0.0f;

	/** Movement speed in Unreal Units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "SplineFollower")
	float Speed = 100.0f;

	/** If the spline is not a closed loop, should it bounce back and forth at the ends? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower")
	bool bBackAndForthLoop = false;

	/** Should the target component rotate to match the spline's rotation at the current distance? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower")
	bool bMatchRotation = true;

	/** If true, moving the component won't generate physics velocity (useful for teleporting or kinematic platforms). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower")
	bool bTeleportPhysics = false;

	/** Tick in editor (Useful for previewing paths in the editor). */
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "SplineFollower", AdvancedDisplay)
	bool bShouldTickInEditor = false;

	/** Fired when the component reaches the end of the spline and changes direction (Ping-Pong mode). */
	UPROPERTY(BlueprintAssignable, Category = "SplineFollower")
	FKzSimpleEventSignature OnDirectionChanged;

	/** Fired when the component reaches the end or the beginning of a non-looping spline. */
	UPROPERTY(BlueprintAssignable, Category = "SplineFollower")
	FKzSimpleEventSignature OnSplineEndReached;

	/** Fired when the component completes a full lap on a closed loop spline. */
	UPROPERTY(BlueprintAssignable, Category = "SplineFollower")
	FKzSimpleEventSignature OnSplineLoopCompleted;

protected:

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> UpdatedComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> SplineComponent = nullptr;

	float SplineLength = 0.0f;

	UPROPERTY(Replicated)
	float CurrentDistance = 0.0f;

	/** Maximum delta time allowed to prevent the component from skipping entire loops during severe lag spikes. */
	static constexpr float MaxTickTime = 0.3f;

public:
	UKzSplineFollowerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Forces the component to find its references and reset its position based on StartAlpha. */
	UFUNCTION(BlueprintCallable, Category = "SplineFollower")
	void Reset();

	UFUNCTION(BlueprintPure, Category = "SplineFollower")
	USplineComponent* GetSpline() const { return SplineComponent; }

	UFUNCTION(BlueprintPure, Category = "SplineFollower")
	float GetCurrentDistance() const { return CurrentDistance; }

private:
	void InitializeReferences();
};