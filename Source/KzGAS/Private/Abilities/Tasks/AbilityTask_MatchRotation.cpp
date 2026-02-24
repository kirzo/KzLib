// Copyright 2026 kirzo

#include "Abilities/Tasks/AbilityTask_MatchRotation.h"
#include "Curves/CurveFloat.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_MatchRotation)

UAbilityTask_MatchRotation::UAbilityTask_MatchRotation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
	bSimulatedTask = true;
	bIsFinished = false;
}

UAbilityTask_MatchRotation* UAbilityTask_MatchRotation::MatchRotation(UGameplayAbility* OwningAbility, FName TaskInstanceName, FRotator Rotation, float Duration, UCurveFloat* OptionalInterpolationCurve)
{
	UAbilityTask_MatchRotation* Task = NewAbilityTask<UAbilityTask_MatchRotation>(OwningAbility, TaskInstanceName);

	if (Task->GetAvatarActor() != nullptr)
	{
		Task->StartRotation = Task->GetAvatarActor()->GetActorQuat();
	}

	Task->TargetRotation = Rotation.Quaternion();
	Task->DurationOfMovement = FMath::Max(Duration, 0.001f);		// Avoid negative or divide-by-zero cases
	Task->TimeMoveStarted = Task->GetWorld()->GetTimeSeconds();
	Task->TimeMoveWillEnd = Task->TimeMoveStarted + Task->DurationOfMovement;
	Task->LerpCurve = OptionalInterpolationCurve;

	return Task;
}

void UAbilityTask_MatchRotation::Activate()
{
}

void UAbilityTask_MatchRotation::InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent)
{
	Super::InitSimulatedTask(InGameplayTasksComponent);

	TimeMoveStarted = GetWorld()->GetTimeSeconds();
	TimeMoveWillEnd = TimeMoveStarted + DurationOfMovement;
}

void UAbilityTask_MatchRotation::TickTask(float DeltaTime)
{
	if (bIsFinished)
	{
		return;
	}

	Super::TickTask(DeltaTime);
	AActor* MyActor = GetAvatarActor();
	if (MyActor)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();

		if (CurrentTime >= TimeMoveWillEnd)
		{
			bIsFinished = true;

			// Teleport in attempt to find a valid collision spot
			MyActor->TeleportTo(MyActor->GetActorLocation(), TargetRotation.Rotator());
			if (!bIsSimulating)
			{
				MyActor->ForceNetUpdate();
				if (ShouldBroadcastAbilityTaskDelegates())
				{
					OnTargetRotationReached.Broadcast();
				}
				EndTask();
			}
		}
		else
		{
			float MoveFraction = (CurrentTime - TimeMoveStarted) / DurationOfMovement;
			if (LerpCurve)
			{
				MoveFraction = LerpCurve->GetFloatValue(MoveFraction);
			}

			FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, MoveFraction);

			MyActor->SetActorRotation(NewRotation);
		}
	}
	else
	{
		bIsFinished = true;
		EndTask();
	}
}

void UAbilityTask_MatchRotation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(UAbilityTask_MatchRotation, StartRotation);
	DOREPLIFETIME(UAbilityTask_MatchRotation, TargetRotation);
	DOREPLIFETIME(UAbilityTask_MatchRotation, DurationOfMovement);
	DOREPLIFETIME(UAbilityTask_MatchRotation, LerpCurve);
}