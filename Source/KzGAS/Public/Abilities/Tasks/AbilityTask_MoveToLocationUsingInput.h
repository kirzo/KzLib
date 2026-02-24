// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Core/KzMovementTypes.h"
#include "AbilityTask_MoveToLocationUsingInput.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoveToLocationInputDelegate, bool, bWasTeleported);

/**
 * Moves the avatar to a target location by simulating movement input,
 * and optionally rotates to face a specific direction upon arrival.
 */
UCLASS()
class KZGAS_API UAbilityTask_MoveToLocationUsingInput : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_MoveToLocationUsingInput(const FObjectInitializer& ObjectInitializer);

	/** Delegate fired when the character successfully reaches the target location. */
	UPROPERTY(BlueprintAssignable)
	FMoveToLocationInputDelegate OnTargetReached;

	/**
	 * Moves the avatar to a target location by adding movement input, optionally rotating upon arrival.
	 * @param Location The target world location.
	 * @param VerticalAlignment Controls how the Z-axis (height) is handled.
	 * @param AcceptanceRadius How close the character needs to be to consider the target reached.
	 * @param Timeout Maximum time in seconds to attempt movement/rotation. If exceeded, the character is teleported.
	 * @param bBlockPlayerInput If true, the player's movement input is ignored during the task.
	 * @param bUseTargetRotation If true, the character will rotate to TargetRotation after reaching the location.
	 * @param TargetRotation The target world rotation to face (only used if bUseTargetRotation is true).
	 * @param InterpolationDuration Time in seconds to interpolate to the exact location (and rotation) once the AcceptanceRadius is reached.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_MoveToLocationUsingInput* MoveToLocationUsingInput(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		FVector Location,
		EKzTargetVerticalAlignment VerticalAlignment = EKzTargetVerticalAlignment::AlignFeetToTarget,
		float AcceptanceRadius = 0.0f,
		float Timeout = 4.0f,
		bool bBlockPlayerInput = true,
		bool bUseTargetRotation = false,
		FRotator TargetRotation = FRotator::ZeroRotator,
		float InterpolationDuration = 0.1f
	);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool AbilityIsEnding) override;

protected:
	UPROPERTY(Replicated)
	FVector TargetLocation;

	UPROPERTY(Replicated)
	FRotator TargetRotation;

	UPROPERTY(Replicated)
	bool bUseTargetRotation;

	UPROPERTY(Replicated)
	float AcceptanceRadius;

	UPROPERTY(Replicated)
	float Timeout;

	UPROPERTY(Replicated)
	float InterpolationDuration;

	UPROPERTY(Replicated)
	bool bBlockInput;

	float TimeStarted;
	bool bIsFinished;

	// --- Interpolation Phase State ---
	bool bIsInterpolatingPhase;
	float TimeInterpolationStarted;
	FVector StartLocationForInterpolation;
	FQuat StartRotationQuat;
	FQuat TargetRotationQuat;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};