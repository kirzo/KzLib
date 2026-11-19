// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Math/Interval.h"
#include "Tickable.h"
#include "UObject/Interface.h"
#include "UObject/Object.h"
#include "Tickets/KzTicketPool.h"
#include "KzTicketDispatcher.generated.h"

/** Something a participant could do with tickets of a type. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzTicketOption
{
	GENERATED_BODY()

	/** What it is, for the participant to know what it was granted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets")
	FGameplayTag Tag;

	/** How many tickets it takes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (ClampMin = "1"))
	int32 Cost = 1;

	/** How likely it is against the participant's other options. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (ClampMin = "0"))
	float Weight = 1.0f;
};

UINTERFACE(MinimalAPI, BlueprintType)
class UKzTicketParticipant : public UInterface
{
	GENERATED_BODY()
};

/** Whoever a ticket dispatcher hands tickets to. */
class KZLIB_API IKzTicketParticipant
{
	GENERATED_BODY()

public:
	/** What it could do right now with tickets of a type; none when it can't or won't. */
	UFUNCTION(BlueprintNativeEvent, Category = "KzLib|Tickets")
	void GetTicketOptions(UPARAM(meta = (Categories = "Ticket")) FGameplayTag Type, TArray<FKzTicketOption>& Options);

	/** It was chosen for one of its options, and holds its tickets until it releases the handle. */
	UFUNCTION(BlueprintNativeEvent, Category = "KzLib|Tickets")
	void OnTicketsGranted(const FKzTicketOption& Option, const FKzTicketHandle& Handle);
};

/** A participant that can take tickets in a dispatch. */
struct FKzTicketCandidate
{
	UObject* Participant = nullptr;

	/** The ones whose cost fits the tickets that are free. */
	TArray<FKzTicketOption> Options;

	/** It was chosen in the last dispatch of the same type. */
	bool bChosenLastTime = false;
};

/** How a dispatcher chooses who gets tickets, and for which of its options. */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KZLIB_API UKzTicketSelection : public UObject
{
	GENERATED_BODY()

public:
	/** Which candidate is chosen next; INDEX_NONE for none. */
	virtual int32 PickCandidate(TConstArrayView<FKzTicketCandidate> Candidates) const PURE_VIRTUAL(UKzTicketSelection::PickCandidate, return INDEX_NONE;);

	/** Which of the candidate's options it gets. */
	virtual int32 PickOption(const FKzTicketCandidate& Candidate) const PURE_VIRTUAL(UKzTicketSelection::PickOption, return INDEX_NONE;);
};

/** Candidates at random, all alike but for those chosen last time; their options at random by weight. */
UCLASS(DisplayName = "Weighted")
class KZLIB_API UKzTicketSelection_Weighted : public UKzTicketSelection
{
	GENERATED_BODY()

public:
	/** Weighs those chosen in the last dispatch of the same type: below 1 they are less likely to go twice in a row, never at 0 unless alone. */
	UPROPERTY(EditAnywhere, Category = "Selection", meta = (ClampMin = "0"))
	float RepeatMultiplier = 0.5f;

	virtual int32 PickCandidate(TConstArrayView<FKzTicketCandidate> Candidates) const override;
	virtual int32 PickOption(const FKzTicketCandidate& Candidate) const override;
};

/** How a dispatcher hands tickets out. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzTicketDispatcherSettings
{
	GENERATED_BODY()

	/** How often each type of ticket is handed out, at random in its range each time. A type that isn't here is only had by acquiring it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (Categories = "Ticket"))
	TMap<FGameplayTag, FFloatInterval> Intervals;

	/** From Start to the first dispatch of every type, at random in this range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets")
	FFloatInterval InitialDelay = FFloatInterval(0.0f, 0.0f);

	/** How many participants a dispatch chooses at most, even with tickets left. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (ClampMin = "1"))
	int32 MaxPerDispatch = 1;

	/** For this long after Start, Start Max Per Dispatch is the limit instead, so it doesn't open with a burst. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (ClampMin = "0", Units = "Seconds"))
	float StartLimitDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (ClampMin = "1"))
	int32 StartMaxPerDispatch = 1;

	/** Who gets tickets and for what. None picks them all alike, with options by weight. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Tickets")
	TObjectPtr<UKzTicketSelection> Selection;
};

/**
 * Hands out the tickets of a pool, every so often for each type, among the participants registered with it: it asks each what it could
 * do with them, chooses as many as the dispatch allows and the tickets pay for, and tells them all at once. A participant takes one grant at
 * a time, and is a candidate again once its tickets are back. Others can still acquire from the same pool directly. Server only.
 */
UCLASS(BlueprintType)
class KZLIB_API UKzTicketDispatcher : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** A dispatcher for a pool, owned by Outer, whose world gives it the time. It starts stopped. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets", meta = (DefaultToSelf = "Outer"))
	static UKzTicketDispatcher* CreateTicketDispatcher(UObject* Outer, UKzTicketPool* Pool, const FKzTicketDispatcherSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void ApplySettings(const FKzTicketDispatcherSettings& NewSettings);

	const FKzTicketDispatcherSettings& GetSettings() const { return Settings; }

	UFUNCTION(BlueprintPure, Category = "KzLib|Tickets")
	UKzTicketPool* GetPool() const { return Pool; }

	/** Adds a participant, an object that implements Kz Ticket Participant. It is dropped by itself once destroyed. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void Register(UObject* Participant);

	/** Removes a participant. Tickets it holds stay its own until it releases them. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void Unregister(UObject* Participant);

	/** Starts handing out after Initial Delay, as if for the first time. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void Start();

	/** Stops handing out. Tickets already granted are untouched. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void Stop();

	UFUNCTION(BlueprintPure, Category = "KzLib|Tickets")
	bool IsRunning() const { return bRunning; }

	/** Hands out tickets of a type now, without waiting for its turn, such as tickets given back unused. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void DispatchNow(UPARAM(meta = (Categories = "Ticket")) FGameplayTag Type);

	virtual UWorld* GetWorld() const override;

	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override { return bRunning; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKzTicketDispatcher, STATGROUP_Tickables); }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

private:
	struct FParticipant
	{
		TWeakObjectPtr<UObject> Object;
		FKzTicketHandle Grant;
	};

	double GetTime() const;
	void Dispatch(FGameplayTag Type);

	UPROPERTY()
	TObjectPtr<UKzTicketPool> Pool;

	UPROPERTY()
	FKzTicketDispatcherSettings Settings;

	TArray<FParticipant> Participants;

	/** When each type is next handed out. */
	TMap<FGameplayTag, double> NextDispatch;

	/** Who was chosen in the last dispatch of each type. */
	TMap<FGameplayTag, TArray<TWeakObjectPtr<UObject>>> LastChosen;

	double StartTime = 0.0;
	bool bRunning = false;
};