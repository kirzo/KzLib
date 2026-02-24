// Copyright 2026 kirzo

#include "Abilities/Tasks/AbilityTask_MoveToLocationUsingInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/MovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_MoveToLocationUsingInput)

UAbilityTask_MoveToLocationUsingInput::UAbilityTask_MoveToLocationUsingInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
	bSimulatedTask = true;
	bIsFinished = false;
	bIsInterpolatingPhase = false;
}

UAbilityTask_MoveToLocationUsingInput* UAbilityTask_MoveToLocationUsingInput::MoveToLocationUsingInput(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName,
	FVector Location,
	EKzTargetVerticalAlignment VerticalAlignment,
	float AcceptanceRadius,
	float Timeout,
	bool bBlockPlayerInput,
	bool bUseTargetRotation,
	FRotator TargetRotation,
	float InterpolationDuration)
{
	UAbilityTask_MoveToLocationUsingInput* MyObj = NewAbilityTask<UAbilityTask_MoveToLocationUsingInput>(OwningAbility, TaskInstanceName);

	AActor* Avatar = MyObj->GetAvatarActor();
	if (Avatar)
	{
		FVector FinalTargetLocation = Location;

		switch (VerticalAlignment)
		{
		case EKzTargetVerticalAlignment::KeepStartZ:
		{
			FinalTargetLocation.Z = Avatar->GetActorLocation().Z;
			break;
		}
		case EKzTargetVerticalAlignment::AlignFeetToTarget:
		{
			const FVector UpDir = Avatar->GetActorUpVector();
			const float HalfHeight = Avatar->GetSimpleCollisionHalfHeight();

			// Offset the target upwards by half-height
			FinalTargetLocation += (UpDir * HalfHeight);
			break;
		}

		case EKzTargetVerticalAlignment::UseTargetZ:
		default:
			// Use the raw location provided
			break;
		}

		MyObj->TargetLocation = FinalTargetLocation;
	}
	else
	{
		// Fallback if avatar is not ready
		MyObj->TargetLocation = Location;
	}

	MyObj->bUseTargetRotation = bUseTargetRotation;
	MyObj->TargetRotation = TargetRotation;
	MyObj->AcceptanceRadius = FMath::Max(AcceptanceRadius, 4.0f);
	MyObj->Timeout = Timeout;
	MyObj->InterpolationDuration = FMath::Max(InterpolationDuration, 0.0f);
	MyObj->bBlockInput = bBlockPlayerInput;

	return MyObj;
}

void UAbilityTask_MoveToLocationUsingInput::Activate()
{
	Super::Activate();
	TimeStarted = GetWorld()->GetTimeSeconds();

	// Safely block player movement input if requested
	if (bBlockInput)
	{
		if (APawn* AvatarPawn = Cast<APawn>(GetAvatarActor()))
		{
			if (APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController()))
			{
				PC->SetIgnoreMoveInput(true);
				PC->SetIgnoreLookInput(true);
			}
		}
	}
}

void UAbilityTask_MoveToLocationUsingInput::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (bIsFinished) return;

	APawn* AvatarPawn = Cast<APawn>(GetAvatarActor());
	if (!AvatarPawn)
	{
		bIsFinished = true;
		EndTask();
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	// Check for Timeout Fallback
	if (Timeout > 0.0f && (CurrentTime - TimeStarted) >= Timeout)
	{
		bIsFinished = true;

		AvatarPawn->TeleportTo(TargetLocation, TargetRotation);

		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnTargetReached.Broadcast(true);
		}
		EndTask();
		return;
	}

	if (!bIsInterpolatingPhase)
	{
		// Calculate Distance to Target (ignoring Z)
		FVector CurrentLocation = AvatarPawn->GetActorLocation();
		FVector DirectionToTarget = TargetLocation - CurrentLocation;
		DirectionToTarget.Z = 0.0f;
		const float DistanceToTargetSq = DirectionToTarget.SizeSquared();

		// Check if reached naturally
		if (DistanceToTargetSq <= FMath::Square(AcceptanceRadius))
		{
			if (UMovementComponent* MovementComponent = AvatarPawn->FindComponentByClass< UMovementComponent>())
			{
				MovementComponent->StopMovementImmediately();
			}

			if (bUseTargetRotation)
			{
				// Enter rotation phase
				bIsInterpolatingPhase = true;
				TimeInterpolationStarted = CurrentTime;
				StartLocationForInterpolation = CurrentLocation;
				StartRotationQuat = AvatarPawn->GetActorQuat();
				TargetRotationQuat = TargetRotation.Quaternion();
			}
			else
			{
				// Finish immediately
				bIsFinished = true;
				if (ShouldBroadcastAbilityTaskDelegates()) OnTargetReached.Broadcast(false);
				EndTask();
			}
		}
		else
		{
			// Inject Input
			const FVector NormalizedDirection = DirectionToTarget.GetSafeNormal();
			AvatarPawn->AddMovementInput(NormalizedDirection, 1.0f, true);
		}
	}
	else
	{
		if (InterpolationDuration <= 0.0f)
		{
			AvatarPawn->SetActorRotation(TargetRotation);
			bIsFinished = true;
			if (ShouldBroadcastAbilityTaskDelegates()) OnTargetReached.Broadcast(false);
			EndTask();
			return;
		}

		float Alpha = (CurrentTime - TimeInterpolationStarted) / InterpolationDuration;
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

		FVector NewLocation = FMath::Lerp(StartLocationForInterpolation, TargetLocation, Alpha);
		FQuat NewRotation = FQuat::Slerp(StartRotationQuat, TargetRotationQuat, Alpha);
		AvatarPawn->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);

		if (Alpha >= 1.0f)
		{
			bIsFinished = true;
			if (ShouldBroadcastAbilityTaskDelegates()) OnTargetReached.Broadcast(false);
			EndTask();
		}
		return;
	}
}

void UAbilityTask_MoveToLocationUsingInput::OnDestroy(bool AbilityIsEnding)
{
	// Guarantee that input is restored regardless of how the task ends (Timeout, Cancelled, Completed)
	if (bBlockInput)
	{
		if (APawn* AvatarPawn = Cast<APawn>(GetAvatarActor()))
		{
			if (APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController()))
			{
				PC->SetIgnoreMoveInput(false);
				PC->SetIgnoreLookInput(false);
			}
		}
	}

	Super::OnDestroy(AbilityIsEnding);
}

void UAbilityTask_MoveToLocationUsingInput::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilityTask_MoveToLocationUsingInput, TargetLocation);
	DOREPLIFETIME(UAbilityTask_MoveToLocationUsingInput, TargetRotation);
	DOREPLIFETIME(UAbilityTask_MoveToLocationUsingInput, bUseTargetRotation);
	DOREPLIFETIME(UAbilityTask_MoveToLocationUsingInput, AcceptanceRadius);
	DOREPLIFETIME(UAbilityTask_MoveToLocationUsingInput, Timeout);
	DOREPLIFETIME(UAbilityTask_MoveToLocationUsingInput, InterpolationDuration);
	DOREPLIFETIME(UAbilityTask_MoveToLocationUsingInput, bBlockInput);
}