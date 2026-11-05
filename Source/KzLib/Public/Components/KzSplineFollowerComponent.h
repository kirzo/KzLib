// Copyright 2026 kirzo

#pragma once

#include "KzLibMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/KzComponentReference.h"
#include "UObject/ObjectKey.h"
#include "KzSplineFollowerComponent.generated.h"

class AKzSplineLimit;
class AKzSplineMarker;
class USplineComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzSplineLimitSignature, AKzSplineLimit*, Limit);

UENUM(BlueprintType)
enum class EKzSplineFollowMode : uint8
{
	/** Moves along the spline at a constant speed. */
	Speed,

	/** Stays at the point of the spline closest to a target actor. */
	Projection,

	/** Doesn't tick: whoever owns it moves it with ProjectTo, such as a camera rig with its focus. */
	Driven
};

/** Component that automatically moves a specified SceneComponent along a SplineComponent, telling the spline's markers when it crosses them. */
UCLASS(ClassGroup = ("Movement"), meta = (BlueprintSpawnableComponent))
class KZLIB_API UKzSplineFollowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** The component that will be moved along the spline. If empty, the Owner's RootComponent will be used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower")
	FKzComponentReference TargetComponentRef;

	/** The Spline to follow. If empty, it will search for the first USplineComponent in the Owner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (AllowedClasses = "/Script/Engine.SplineComponent"))
	FKzComponentReference SplineComponentRef;

	/** How the follower advances along the spline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SplineFollower")
	EKzSplineFollowMode Mode = EKzSplineFollowMode::Speed;

	/** Normalized starting position on the spline (0.0 to 1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "Mode == EKzSplineFollowMode::Speed", EditConditionHides))
	float StartAlpha = 0.0f;

	/** Movement speed in Unreal Units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "SplineFollower", meta = (EditCondition = "Mode == EKzSplineFollowMode::Speed", EditConditionHides))
	float Speed = 100.0f;

	/** If the spline is not a closed loop, should it bounce back and forth at the ends? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (EditCondition = "Mode == EKzSplineFollowMode::Speed", EditConditionHides))
	bool bBackAndForthLoop = false;

	/** Actor whose closest spline point the follower stays at. In the editor it previews the follower without playing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (EditCondition = "Mode == EKzSplineFollowMode::Projection", EditConditionHides))
	TObjectPtr<AActor> ProjectionTarget;

	/** In game, project the first local player's pawn instead of ProjectionTarget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (EditCondition = "Mode == EKzSplineFollowMode::Projection", EditConditionHides))
	bool bProjectPlayerPawn = true;

	/** A target that moves farther than this in one frame teleports the follower instead of moving it. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (ClampMin = "0", Units = "Centimeters", EditCondition = "Mode == EKzSplineFollowMode::Projection", EditConditionHides))
	float TeleportDistance = 1000.0f;

	/** Should the target component rotate to match the spline's rotation at the current distance? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower")
	bool bMatchRotation = true;

	/** Offset from the spline point, in the spline's frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (EditCondition = "bMatchRotation"))
	FVector LocationOffset = FVector::ZeroVector;

	/** Rotation added on top of the spline's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", meta = (EditCondition = "bMatchRotation"))
	FRotator RotationOffset = FRotator::ZeroRotator;

	/** If true, moving the component won't generate physics velocity (useful for teleporting or kinematic platforms). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower")
	bool bTeleportPhysics = false;

	/** Tick in editor (Useful for previewing paths in the editor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", AdvancedDisplay)
	bool bShouldTickInEditor = false;

	/** Enabled limits also block the follower in the editor, never sticking, to help place them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SplineFollower", AdvancedDisplay)
	bool bApplyLimitsInEditor = false;

	/** Fired when an enabled limit stops the follower. */
	UPROPERTY(BlueprintAssignable, Category = "SplineFollower")
	FKzSplineLimitSignature OnLimitReached;

	/** Fired when the component reaches the end of the spline and changes direction (Ping-Pong mode). */
	UPROPERTY(BlueprintAssignable, Category = "SplineFollower")
	FKzSimpleEventSignature OnDirectionChanged;

	/** Fired when the component reaches the end or the beginning of a non-looping spline. */
	UPROPERTY(BlueprintAssignable, Category = "SplineFollower")
	FKzSimpleEventSignature OnSplineEndReached;

	/** Fired when the component completes a full lap on a closed loop spline. */
	UPROPERTY(BlueprintAssignable, Category = "SplineFollower")
	FKzSimpleEventSignature OnSplineLoopCompleted;

	/** Fired when the follower is reset back to its starting position (Reset, or a subclass reset), with the reset already applied. */
	UPROPERTY(BlueprintAssignable, Category = "SplineFollower")
	FKzSimpleEventSignature OnReset;

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
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Forces the component to find its references and reset its position based on StartAlpha. Server-authoritative. */
	UFUNCTION(BlueprintCallable, Category = "SplineFollower")
	void Reset();

	/**
	 * Sets the current distance along the spline, applying loop/bounce/clamp rules based on the current mode.
	 * Snaps the target component to the new location immediately. Server-authoritative.
	 */
	UFUNCTION(BlueprintCallable, Category = "SplineFollower")
	void SetDistance(float NewDistance);

	/**
	 * Sets the current position along the spline as a normalized alpha (0.0 to 1.0).
	 * Snaps the target component to the new location immediately. Server-authoritative.
	 */
	UFUNCTION(BlueprintCallable, Category = "SplineFollower")
	void SetAlpha(float NewAlpha);

	UFUNCTION(BlueprintPure, Category = "SplineFollower")
	USplineComponent* GetSpline() const { return SplineComponent; }

	UFUNCTION(BlueprintPure, Category = "SplineFollower")
	float GetCurrentDistance() const { return CurrentDistance; }

	/** The transform the follower gives its component at Distance, offsets included. */
	UFUNCTION(BlueprintPure, Category = "SplineFollower")
	FTransform GetTransformAtDistance(float Distance) const;

	/** True when the last update jumped (start, SetDistance, a teleported target) instead of moving continuously. */
	UFUNCTION(BlueprintPure, Category = "SplineFollower")
	bool WasTeleported() const { return bTeleported; }

	/** The actor Projection mode follows: the player pawn in game when bProjectPlayerPawn, ProjectionTarget otherwise. */
	UFUNCTION(BlueprintPure, Category = "SplineFollower")
	AActor* GetProjectedActor() const;

	/**
	 * Moves to the point of the spline closest to Location, with the limits and markers of a normal move.
	 * Searched near the current distance unless it teleports. Meant for Driven mode.
	 */
	UFUNCTION(BlueprintCallable, Category = "SplineFollower")
	void ProjectTo(const FVector& Location, bool bTeleport, float DeltaTime);

private:
	bool bTeleported = false;
	bool bPendingTeleport = true;

	TWeakObjectPtr<const AActor> LastProjectedActor;
	FVector LastProjectedLocation = FVector::ZeroVector;

	/** Whether each marker of the spline was behind the follower last frame. */
	TMap<TObjectKey<AKzSplineMarker>, bool> MarkerSides;

	/** Sticky limit holding the follower. */
	TWeakObjectPtr<AKzSplineLimit> StuckLimit;

	/** Limit that stopped the follower last frame, so OnLimitReached fires once per stop. */
	TWeakObjectPtr<AKzSplineLimit> LastHitLimit;

	void InitializeReferences();

	/** Clamps a move from PrevDistance to NewDistance with the enabled limits around PrevDistance. */
	float ApplyLimits(float PrevDistance, float NewDistance);

	/** Notifies the markers whose side changed since last frame, in the order the follower meets them. */
	void UpdateMarkers();

	bool CanMove() const;

	/** Starts a move: settles the teleport, refreshes the markers in the editor. Returns whether a sticky limit holds the follower. */
	bool BeginMove(bool bTeleport);

	/** Distance of the spline point closest to Location, searched near the current distance unless teleported. */
	float ProjectLocation(const FVector& Location) const;

	/** Applies the limits, commits the distance and moves the component. Returns whether a limit stopped it. */
	bool FinishMove(float NewDistance, float DeltaTime, bool bStuck);

	/** Moves the target component to Distance. A zero DeltaTime is a teleport: no induced velocity. */
	void MoveToDistance(float Distance, float DeltaTime);
};
