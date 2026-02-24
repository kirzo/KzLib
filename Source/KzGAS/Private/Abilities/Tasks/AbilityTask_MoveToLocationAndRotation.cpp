// Copyright 2026 kirzo

#include "Abilities/Tasks/AbilityTask_MoveToLocationAndRotation.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_MoveToLocationAndRotation)

UAbilityTask_MoveToLocationAndRotation::UAbilityTask_MoveToLocationAndRotation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
	bSimulatedTask = true;
	bIsFinished = false;
}

UAbilityTask_MoveToLocationAndRotation* UAbilityTask_MoveToLocationAndRotation::MoveToLocationAndRotation(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName,
	FVector Location,
	FRotator Rotation,
	float Duration,
	EKzTargetVerticalAlignment VerticalAlignment,
	UCurveFloat* OptionalInterpolationCurve,
	UCurveVector* OptionalVectorInterpolationCurve)
{
	UAbilityTask_MoveToLocationAndRotation* MyObj = NewAbilityTask<UAbilityTask_MoveToLocationAndRotation>(OwningAbility, TaskInstanceName);

	AActor* Avatar = MyObj->GetAvatarActor();
	if (Avatar)
	{
		MyObj->StartLocation = Avatar->GetActorLocation();
		MyObj->StartRotation = Avatar->GetActorQuat();

		FVector FinalTargetLocation = Location;

		switch (VerticalAlignment)
		{
			case EKzTargetVerticalAlignment::KeepStartZ:
			{
				FinalTargetLocation.Z = MyObj->StartLocation.Z;
				break;
			}
			case EKzTargetVerticalAlignment::AlignFeetToTarget:
			{
				if (ACharacter* Character = Cast<ACharacter>(Avatar))
				{
					if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
					{
						const FVector UpDir = Character->GetActorUpVector();
						const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

						// Offset the target upwards by half-height
						FinalTargetLocation += (UpDir * HalfHeight);
					}
				}
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

	MyObj->TargetRotation = Rotation.Quaternion();
	MyObj->DurationOfMovement = FMath::Max(Duration, 0.001f); // Avoid divide-by-zero
	MyObj->TimeMoveStarted = MyObj->GetWorld()->GetTimeSeconds();
	MyObj->TimeMoveWillEnd = MyObj->TimeMoveStarted + MyObj->DurationOfMovement;
	MyObj->LerpCurve = OptionalInterpolationCurve;
	MyObj->LerpCurveVector = OptionalVectorInterpolationCurve;

	return MyObj;
}

void UAbilityTask_MoveToLocationAndRotation::Activate()
{
	Super::Activate();

	// Set movement mode to Custom to avoid physics interference (Gravity, Collision correction)
	ACharacter* MyCharacter = Cast<ACharacter>(GetAvatarActor());
	if (MyCharacter)
	{
		UCharacterMovementComponent* CharMoveComp = Cast<UCharacterMovementComponent>(MyCharacter->GetMovementComponent());
		if (CharMoveComp)
		{
			CharMoveComp->SetMovementMode(MOVE_None);
		}
	}
}

void UAbilityTask_MoveToLocationAndRotation::InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent)
{
	Super::InitSimulatedTask(InGameplayTasksComponent);

	TimeMoveStarted = GetWorld()->GetTimeSeconds();
	TimeMoveWillEnd = TimeMoveStarted + DurationOfMovement;
}

void UAbilityTask_MoveToLocationAndRotation::TickTask(float DeltaTime)
{
	if (bIsFinished)
	{
		return;
	}

	Super::TickTask(DeltaTime);

	AActor* MyActor = GetAvatarActor();
	if (MyActor)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();

		if (CurrentTime >= TimeMoveWillEnd)
		{
			bIsFinished = true;

			// Teleport in attempt to find a valid collision spot
			MyActor->TeleportTo(TargetLocation, TargetRotation.Rotator());

			if (!bIsSimulating)
			{
				MyActor->ForceNetUpdate();
				if (ShouldBroadcastAbilityTaskDelegates())
				{
					OnTargetReached.Broadcast();
				}
				EndTask();
			}
		}
		else
		{
			FVector NewLocation;
			FRotator NewRotation;

			float MoveFraction = (CurrentTime - TimeMoveStarted) / DurationOfMovement;
			MoveFraction = FMath::Clamp(MoveFraction, 0.0f, 1.0f);

			if (LerpCurveVector)
			{
				const FVector ComponentInterpolationFraction = LerpCurveVector->GetVectorValue(MoveFraction);
				NewLocation = FMath::Lerp<FVector, FVector>(StartLocation, TargetLocation, ComponentInterpolationFraction);
			}
			else
			{
				float LocationAlpha = MoveFraction;
				if (LerpCurve)
				{
					LocationAlpha = LerpCurve->GetFloatValue(MoveFraction);
				}

				NewLocation = FMath::Lerp<FVector, float>(StartLocation, TargetLocation, LocationAlpha);
			}

			float RotationAlpha = MoveFraction;
			if (LerpCurve)
			{
				RotationAlpha = LerpCurve->GetFloatValue(MoveFraction);
			}

			NewRotation = FQuat::Slerp(StartRotation, TargetRotation, RotationAlpha).Rotator();

			MyActor->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
	else
	{
		bIsFinished = true;
		EndTask();
	}
}

void UAbilityTask_MoveToLocationAndRotation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilityTask_MoveToLocationAndRotation, StartLocation);
	DOREPLIFETIME(UAbilityTask_MoveToLocationAndRotation, TargetLocation);
	DOREPLIFETIME(UAbilityTask_MoveToLocationAndRotation, StartRotation);
	DOREPLIFETIME(UAbilityTask_MoveToLocationAndRotation, TargetRotation);
	DOREPLIFETIME(UAbilityTask_MoveToLocationAndRotation, DurationOfMovement);
	DOREPLIFETIME(UAbilityTask_MoveToLocationAndRotation, LerpCurve);
	DOREPLIFETIME(UAbilityTask_MoveToLocationAndRotation, LerpCurveVector);
}

void UAbilityTask_MoveToLocationAndRotation::OnDestroy(bool AbilityIsEnding)
{
	if (!bIsFinished)
	{
		AActor* MyActor = GetAvatarActor();
		if (MyActor)
		{
			ACharacter* MyCharacter = Cast<ACharacter>(MyActor);
			if (MyCharacter)
			{
				UCharacterMovementComponent* CharMoveComp = Cast<UCharacterMovementComponent>(MyCharacter->GetMovementComponent());
				if (CharMoveComp && CharMoveComp->MovementMode == MOVE_None)
				{
					CharMoveComp->SetMovementMode(MOVE_Falling);
				}
			}
		}
	}

	Super::OnDestroy(AbilityIsEnding);
}