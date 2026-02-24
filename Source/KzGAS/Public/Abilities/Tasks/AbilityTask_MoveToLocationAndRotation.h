// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Core/KzMovementTypes.h"
#include "AbilityTask_MoveToLocationAndRotation.generated.h"

class UCurveFloat;
class UCurveVector;
class UGameplayTasksComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveToLocationAndRotationDelegate);

/**
 * Move to a location and rotate to a target rotation, ignoring clipping, over a given length of time.
 * Ends when the TargetLocation is reached.
 * This will RESET your character's current movement mode to MOVE_Custom during flight!
 * If you wish to maintain PHYS_Flying or PHYS_Walking, you must reset it manually on completion,
 * although this task attempts to restore MOVE_Falling on destroy.
 */
UCLASS()
class KZGAS_API UAbilityTask_MoveToLocationAndRotation : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_MoveToLocationAndRotation(const FObjectInitializer& ObjectInitializer);

	/** Delegate fired when the target location and rotation are reached */
	UPROPERTY(BlueprintAssignable)
	FMoveToLocationAndRotationDelegate OnTargetReached;

	virtual void InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent) override;

	/**
	 * Moves the avatar to a target location and rotation over a specified duration.
	 * @param Location The target world location.
	 * @param Rotation The target world rotation.
	 * @param Duration Time in seconds to reach the target.
	 * @param VerticalAlignment Controls how the Z-axis (height) is handled relative to the capsule or start location.
	 * @param OptionalInterpolationCurve Optional curve (Float 0-1) to control the speed of the interpolation.
	 * @param OptionalVectorInterpolationCurve Optional curve (Vector) for 3D path control.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_MoveToLocationAndRotation* MoveToLocationAndRotation(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		FVector Location,
		FRotator Rotation,
		float Duration,
		EKzTargetVerticalAlignment VerticalAlignment = EKzTargetVerticalAlignment::UseTargetZ,
		UCurveFloat* OptionalInterpolationCurve = nullptr,
		UCurveVector* OptionalVectorInterpolationCurve = nullptr
	);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool AbilityIsEnding) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	bool bIsFinished;

	UPROPERTY(Replicated)
	FVector StartLocation;

	UPROPERTY(Replicated)
	FVector TargetLocation;

	UPROPERTY(Replicated)
	FQuat StartRotation;

	UPROPERTY(Replicated)
	FQuat TargetRotation;

	UPROPERTY(Replicated)
	float DurationOfMovement;

	float TimeMoveStarted;
	float TimeMoveWillEnd;

	UPROPERTY(Replicated)
	TObjectPtr<UCurveFloat> LerpCurve;

	UPROPERTY(Replicated)
	TObjectPtr<UCurveVector> LerpCurveVector;
};