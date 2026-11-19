// Copyright 2026 kirzo

#include "Tickets/KzTicketDispatcher.h"
#include "Engine/World.h"

namespace
{
	/** An index drawn by weight; uniform when every weight is 0. */
	int32 PickByWeight(TConstArrayView<float> Weights)
	{
		if (Weights.IsEmpty())
		{
			return INDEX_NONE;
		}

		float Total = 0.0f;
		for (const float Weight : Weights)
		{
			Total += FMath::Max(Weight, 0.0f);
		}
		if (Total <= 0.0f)
		{
			return FMath::RandHelper(Weights.Num());
		}

		float Pick = FMath::FRand() * Total;
		for (int32 Index = 0; Index < Weights.Num(); ++Index)
		{
			Pick -= FMath::Max(Weights[Index], 0.0f);
			if (Pick < 0.0f)
			{
				return Index;
			}
		}
		return Weights.Num() - 1;
	}

	float RandRange(const FFloatInterval& Range)
	{
		return FMath::FRandRange(Range.Min, FMath::Max(Range.Min, Range.Max));
	}
}

//----------------------------------------------------------------------//
//  UKzTicketSelection_Weighted
//----------------------------------------------------------------------//

int32 UKzTicketSelection_Weighted::PickCandidate(TConstArrayView<FKzTicketCandidate> Candidates) const
{
	TArray<float, TInlineAllocator<16>> Weights;
	for (const FKzTicketCandidate& Candidate : Candidates)
	{
		Weights.Add(Candidate.bChosenLastTime ? RepeatMultiplier : 1.0f);
	}
	return PickByWeight(Weights);
}

int32 UKzTicketSelection_Weighted::PickOption(const FKzTicketCandidate& Candidate) const
{
	TArray<float, TInlineAllocator<8>> Weights;
	for (const FKzTicketOption& Option : Candidate.Options)
	{
		Weights.Add(Option.Weight);
	}
	return PickByWeight(Weights);
}

//----------------------------------------------------------------------//
//  UKzTicketDispatcher
//----------------------------------------------------------------------//

UKzTicketDispatcher* UKzTicketDispatcher::CreateTicketDispatcher(UObject* Outer, UKzTicketPool* Pool, const FKzTicketDispatcherSettings& Settings)
{
	UKzTicketDispatcher* Dispatcher = NewObject<UKzTicketDispatcher>(Outer ? Outer : GetTransientPackage());
	Dispatcher->Pool = Pool;
	Dispatcher->ApplySettings(Settings);
	return Dispatcher;
}

void UKzTicketDispatcher::ApplySettings(const FKzTicketDispatcherSettings& NewSettings)
{
	Settings = NewSettings;

	// Types that are gone stop; new ones get their turn in Tick
	for (auto It = NextDispatch.CreateIterator(); It; ++It)
	{
		if (!Settings.Intervals.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
}

void UKzTicketDispatcher::Register(UObject* Participant)
{
	if (!Participant || !Participant->Implements<UKzTicketParticipant>())
	{
		return;
	}

	if (!Participants.ContainsByPredicate([Participant](const FParticipant& Entry) { return Entry.Object.Get() == Participant; }))
	{
		Participants.AddDefaulted_GetRef().Object = Participant;
	}
}

void UKzTicketDispatcher::Unregister(UObject* Participant)
{
	Participants.RemoveAll([Participant](const FParticipant& Entry) { return Entry.Object.Get() == Participant; });
}

void UKzTicketDispatcher::Start()
{
	bRunning = true;
	StartTime = GetTime();
	LastChosen.Reset();

	const double First = StartTime + RandRange(Settings.InitialDelay);
	NextDispatch.Reset();
	for (const TPair<FGameplayTag, FFloatInterval>& Pair : Settings.Intervals)
	{
		NextDispatch.Add(Pair.Key, First);
	}
}

void UKzTicketDispatcher::Stop()
{
	bRunning = false;
}

void UKzTicketDispatcher::DispatchNow(FGameplayTag Type)
{
	Dispatch(Type);
}

UWorld* UKzTicketDispatcher::GetWorld() const
{
	const UObject* Outer = GetOuter();
	return Outer && !HasAnyFlags(RF_ClassDefaultObject) ? Outer->GetWorld() : nullptr;
}

void UKzTicketDispatcher::Tick(float DeltaTime)
{
	const double Now = GetTime();

	// Gathered first: a participant told of its grant may change the settings
	TArray<FGameplayTag, TInlineAllocator<4>> Due;
	for (const TPair<FGameplayTag, FFloatInterval>& Pair : Settings.Intervals)
	{
		double& Next = NextDispatch.FindOrAdd(Pair.Key, Now + RandRange(Pair.Value));
		if (Now >= Next)
		{
			Next = Now + RandRange(Pair.Value);
			Due.Add(Pair.Key);
		}
	}

	for (const FGameplayTag& Type : Due)
	{
		Dispatch(Type);
	}
}

ETickableTickType UKzTicketDispatcher::GetTickableTickType() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Conditional;
}

double UKzTicketDispatcher::GetTime() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : FPlatformTime::Seconds();
}

void UKzTicketDispatcher::Dispatch(FGameplayTag Type)
{
	if (!Pool)
	{
		return;
	}

	Participants.RemoveAll([](const FParticipant& Entry) { return !Entry.Object.IsValid(); });

	// Everyone without a grant, with all it could do; costs are matched against the free tickets as they run out
	const TArray<TWeakObjectPtr<UObject>>& Last = LastChosen.FindOrAdd(Type);
	TArray<FKzTicketCandidate> Candidates;
	TArray<TArray<FKzTicketOption>> AllOptions;
	for (FParticipant& Entry : Participants)
	{
		if (Entry.Grant.IsValid())
		{
			if (Pool->IsHeld(Entry.Grant))
			{
				continue;
			}
			Entry.Grant = FKzTicketHandle();
		}

		UObject* Object = Entry.Object.Get();
		TArray<FKzTicketOption> Options;
		IKzTicketParticipant::Execute_GetTicketOptions(Object, Type, Options);
		Options.RemoveAll([](const FKzTicketOption& Option) { return Option.Cost < 1; });
		if (!Options.IsEmpty())
		{
			FKzTicketCandidate& Candidate = Candidates.AddDefaulted_GetRef();
			Candidate.Participant = Object;
			Candidate.bChosenLastTime = Last.Contains(Object);
			AllOptions.Add(MoveTemp(Options));
		}
	}

	struct FChosen
	{
		TWeakObjectPtr<UObject> Participant;
		FKzTicketOption Option;
		FKzTicketHandle Handle;
	};
	TArray<FChosen> Chosen;

	const UKzTicketSelection* Selection = Settings.Selection ? Settings.Selection.Get() : GetDefault<UKzTicketSelection_Weighted>();
	const int32 Limit = GetTime() - StartTime < Settings.StartLimitDuration ? Settings.StartMaxPerDispatch : Settings.MaxPerDispatch;
	while (Chosen.Num() < Limit)
	{
		const int32 Free = Pool->GetCount(Type, EKzTicketState::Free);
		for (int32 Index = Candidates.Num() - 1; Index >= 0; --Index)
		{
			Candidates[Index].Options = AllOptions[Index].FilterByPredicate([Free](const FKzTicketOption& Option) { return Option.Cost <= Free; });
			if (Candidates[Index].Options.IsEmpty())
			{
				Candidates.RemoveAt(Index);
				AllOptions.RemoveAt(Index);
			}
		}

		const int32 Pick = Selection->PickCandidate(Candidates);
		if (!Candidates.IsValidIndex(Pick))
		{
			break;
		}

		const FKzTicketCandidate& Candidate = Candidates[Pick];
		const int32 OptionIndex = Selection->PickOption(Candidate);
		if (!Candidate.Options.IsValidIndex(OptionIndex))
		{
			break;
		}

		FChosen& Choice = Chosen.AddDefaulted_GetRef();
		Choice.Participant = Candidate.Participant;
		Choice.Option = Candidate.Options[OptionIndex];
		Choice.Handle = Pool->TryAcquire(Type, Choice.Option.Cost, Candidate.Participant);
		if (!Choice.Handle.IsValid())
		{
			Chosen.Pop();
			break;
		}

		Candidates.RemoveAt(Pick);
		AllOptions.RemoveAt(Pick);
	}

	TArray<TWeakObjectPtr<UObject>>& Remembered = LastChosen.FindOrAdd(Type);
	Remembered.Reset();
	for (const FChosen& Choice : Chosen)
	{
		Remembered.Add(Choice.Participant);
		if (FParticipant* Entry = Participants.FindByPredicate([&Choice](const FParticipant& Participant) { return Participant.Object == Choice.Participant; }))
		{
			Entry->Grant = Choice.Handle;
		}
	}

	// All at once, once everyone is chosen
	for (const FChosen& Choice : Chosen)
	{
		if (UObject* Object = Choice.Participant.Get())
		{
			IKzTicketParticipant::Execute_OnTicketsGranted(Object, Choice.Option, Choice.Handle);
		}
	}
}