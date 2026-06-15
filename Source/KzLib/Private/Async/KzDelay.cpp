// Copyright 2026 kirzo

#include "Async/KzDelay.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

UKzDelay* UKzDelay::KzDelay(const UObject* InWorldContextObject, float InDuration)
{
	UKzDelay* Action = NewObject<UKzDelay>();
	Action->WorldContextObject = InWorldContextObject;
	Action->Duration = InDuration;
	Action->RegisterWithGameInstance(InWorldContextObject);
	return Action;
}

void UKzDelay::Activate()
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		// No world to time against: cancel out so the graph still continues.
		Finish(false);
		return;
	}

	// Clamp so a zero/negative duration still waits ~one tick (fires async; Cancel can win).
	const float ClampedDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	World->GetTimerManager().SetTimer(TimerHandle, this, &UKzDelay::HandleTimerElapsed, ClampedDuration, /*bLoop=*/false);
}

void UKzDelay::Cancel()
{
	Finish(false);
}

void UKzDelay::HandleTimerElapsed()
{
	Finish(true);
}

void UKzDelay::Finish(bool bCompleted)
{
	// Exactly one pin fires once: a finishing timer and a late Cancel() can't both resolve.
	if (bFinished)
	{
		return;
	}
	bFinished = true;

	if (const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}

	if (bCompleted)
	{
		Completed.Broadcast();
	}
	else
	{
		Cancelled.Broadcast();
	}

	SetReadyToDestroy();
}
