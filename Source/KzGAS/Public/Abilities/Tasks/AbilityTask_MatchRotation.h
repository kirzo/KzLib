// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_MatchRotation.generated.h"

class UCurveFloat;
class UGameplayTasksComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMatchRotateDelegate);

UCLASS()
class KZGAS_API UAbilityTask_MatchRotation : public UAbilityTask
{
	GENERATED_BODY()

	UPROPERTY(BlueprintAssignable)
	FMatchRotateDelegate OnTargetRotationReached;

	UAbilityTask_MatchRotation(const FObjectInitializer& ObjectInitializer);

	virtual void InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent) override;

	/** Rotate to the specified rotation, using the float curve (range 0 - 1) or fallback to linear interpolation */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_MatchRotation* MatchRotation(UGameplayAbility* OwningAbility, FName TaskInstanceName, FRotator Rotation, float Duration, UCurveFloat* OptionalInterpolationCurve);

	virtual void Activate() override;

	/** Tick function for this task, if bTickingTask == true */
	virtual void TickTask(float DeltaTime) override;

protected:
	bool bIsFinished;

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
};