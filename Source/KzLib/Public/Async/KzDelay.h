// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "KzDelay.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKzDelayPin);

/**
 * Cancellable delay. Like the built-in Delay, but with a "Cancel" input exec pin and a Cancelled output:
 * pulse Cancel (or call Cancel() on the returned Delay object) to stop the wait early. Fires Completed
 * when the duration elapses, or Cancelled otherwise. Exactly one fires, once.
 *
 * The HasDedicatedAsyncNode meta routes it to UK2Node_KzCancellableAsyncAction; the default async node skips it.
 */
UCLASS(meta = (HasDedicatedAsyncNode, ExposedAsyncProxy = "Delay"))
class KZLIB_API UKzDelay : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Wait for Duration seconds unless cancelled first. Returns the action object (Delay) so it can be Cancel()'d later. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Flow", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Kz Delay"))
	static UKzDelay* KzDelay(const UObject* WorldContextObject, float Duration = 0.2f);

	/** The full duration elapsed. */
	UPROPERTY(BlueprintAssignable)
	FKzDelayPin Completed;

	/** Cancel() ran before the duration elapsed (or there was no world to time against). */
	UPROPERTY(BlueprintAssignable)
	FKzDelayPin Cancelled;

	/** Stop the pending delay and fire Cancelled. No-op once the delay has already completed or been cancelled. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Flow")
	void Cancel();

	virtual void Activate() override;

private:
	/** Timer callback: the duration elapsed. */
	void HandleTimerElapsed();

	/** Clear the timer, fire the matching pin once, and release the node. */
	void Finish(bool bCompleted);

	/** World context the factory was called with; resolves the timer manager. */
	UPROPERTY(Transient)
	TObjectPtr<const UObject> WorldContextObject = nullptr;

	FTimerHandle TimerHandle;
	float Duration = 0.2f;
	bool bFinished = false;
};
