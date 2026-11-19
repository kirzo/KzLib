// Copyright 2026 kirzo

#include "Tickets/KzTicketPool.h"
#include "Engine/World.h"

UKzTicketPool* UKzTicketPool::CreateTicketPool(UObject* Outer, const FKzTicketPoolSettings& Settings)
{
	UKzTicketPool* Pool = NewObject<UKzTicketPool>(Outer ? Outer : GetTransientPackage());
	Pool->ApplySettings(Settings);
	return Pool;
}

void UKzTicketPool::ApplySettings(const FKzTicketPoolSettings& NewSettings)
{
	Settings = NewSettings;

	for (auto It = Types.CreateIterator(); It; ++It)
	{
		if (!Settings.Types.Contains(It.Key()))
		{
			const FGameplayTag Gone = It.Key();
			Holds.RemoveAll([&Gone](const FHold& Hold) { return Hold.Type == Gone; });
			It.RemoveCurrent();
		}
	}

	for (const TPair<FGameplayTag, FKzTicketTypeSettings>& Pair : Settings.Types)
	{
		FType* Type = Types.Find(Pair.Key);
		if (!Type)
		{
			Type = &Types.Add(Pair.Key);
			Type->Count = FMath::Clamp(Pair.Value.Initial, 0, Pair.Value.Max);
		}
		Type->Settings = Pair.Value;
		Type->NextRecharge = 0.0;

		const int32 Excess = Type->Count - Type->PendingRemovals - FMath::Max(Pair.Value.Max, 0);
		if (Excess > 0)
		{
			RemoveTickets(Pair.Key, Excess);
		}
	}
}

void UKzTicketPool::Reset()
{
	Holds.Reset();
	for (TPair<FGameplayTag, FType>& Pair : Types)
	{
		FType& Type = Pair.Value;
		Type.Count = FMath::Clamp(Type.Settings.Initial, 0, Type.Settings.Max);
		Type.Held = 0;
		Type.CoolingUntil.Reset();
		Type.PendingRemovals = 0;
		Type.NextRecharge = 0.0;
	}
}

bool UKzTicketPool::CanAcquire(FGameplayTag Type, int32 Amount)
{
	Refresh();
	const FType* Found = Types.Find(Type);
	return Found && Amount > 0 && Found->GetFree() >= Amount;
}

FKzTicketHandle UKzTicketPool::TryAcquire(FGameplayTag Type, int32 Amount, UObject* Owner)
{
	FKzTicketHandle Handle;
	if (!CanAcquire(Type, Amount))
	{
		return Handle;
	}

	Types[Type].Held += Amount;

	FHold& Hold = Holds.AddDefaulted_GetRef();
	Hold.Id = NextId++;
	Hold.Type = Type;
	Hold.Amount = Amount;
	Hold.Owner = Owner;
	Hold.bHasOwner = Owner != nullptr;
	Hold.Since = GetTime();

	Handle.Pool = this;
	Handle.Id = Hold.Id;
	return Handle;
}

void UKzTicketPool::Release(FKzTicketHandle& Handle, bool bCooldown)
{
	if (Handle.Pool.Get() == this)
	{
		const int32 Index = Holds.IndexOfByPredicate([&Handle](const FHold& Hold) { return Hold.Id == Handle.Id; });
		if (Index != INDEX_NONE)
		{
			ReleaseHold(Index, bCooldown, GetTime());
		}
	}
	Handle = FKzTicketHandle();
}

bool UKzTicketPool::IsHeld(const FKzTicketHandle& Handle)
{
	if (Handle.Pool.Get() != this)
	{
		return false;
	}

	Refresh();
	return Holds.ContainsByPredicate([&Handle](const FHold& Hold) { return Hold.Id == Handle.Id; });
}

void UKzTicketPool::AddTickets(FGameplayTag Type, int32 Count)
{
	Refresh();
	FType* Found = Types.Find(Type);
	if (!Found || Count <= 0)
	{
		return;
	}

	// Held tickets about to go away stay instead
	const int32 Kept = FMath::Min(Found->PendingRemovals, Count);
	Found->PendingRemovals -= Kept;
	Found->Count = FMath::Min(Found->Count + Count - Kept, FMath::Max(Found->Settings.Max, 0) + Found->PendingRemovals);
}

void UKzTicketPool::RemoveTickets(FGameplayTag Type, int32 Count)
{
	Refresh();
	FType* Found = Types.Find(Type);
	if (!Found || Count <= 0)
	{
		return;
	}

	const int32 FromFree = FMath::Min(Found->GetFree(), Count);
	Found->Count -= FromFree;
	Count -= FromFree;

	// The ones that would be free last go first
	const int32 FromCooling = FMath::Min(Found->CoolingUntil.Num(), Count);
	Found->CoolingUntil.Sort();
	Found->CoolingUntil.RemoveAt(Found->CoolingUntil.Num() - FromCooling, FromCooling);
	Found->Count -= FromCooling;
	Count -= FromCooling;

	Found->PendingRemovals += FMath::Min(Found->Held - Found->PendingRemovals, Count);
}

void UKzTicketPool::StartCooldown(FGameplayTag Type)
{
	Refresh();
	if (FType* Found = Types.Find(Type))
	{
		Cool(*Found, Found->GetFree(), GetTime());
	}
}

int32 UKzTicketPool::GetCount(FGameplayTag Type, EKzTicketState State)
{
	Refresh();
	const FType* Found = Types.Find(Type);
	if (!Found)
	{
		return 0;
	}

	switch (State)
	{
	case EKzTicketState::Free:
		return Found->GetFree();
	case EKzTicketState::Held:
		return Found->Held;
	case EKzTicketState::Cooling:
		return Found->CoolingUntil.Num();
	}
	return 0;
}

TArray<float> UKzTicketPool::GetCooldownsLeft(FGameplayTag Type)
{
	Refresh();
	TArray<float> Left;
	if (const FType* Found = Types.Find(Type))
	{
		const double Now = GetTime();
		for (const double Until : Found->CoolingUntil)
		{
			Left.Add(static_cast<float>(Until - Now));
		}
		Left.Sort();
	}
	return Left;
}

void UKzTicketPool::ReleaseTickets(FKzTicketHandle& Handle, bool bCooldown)
{
	if (UKzTicketPool* Pool = Handle.Pool.Get())
	{
		Pool->Release(Handle, bCooldown);
	}
	Handle = FKzTicketHandle();
}

UWorld* UKzTicketPool::GetWorld() const
{
	const UObject* Outer = GetOuter();
	return Outer && !HasAnyFlags(RF_ClassDefaultObject) ? Outer->GetWorld() : nullptr;
}

double UKzTicketPool::GetTime() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : FPlatformTime::Seconds();
}

void UKzTicketPool::Refresh()
{
	const double Now = GetTime();

	// Holders that are gone or held too long give their tickets back
	for (int32 Index = Holds.Num() - 1; Index >= 0; --Index)
	{
		const FHold& Hold = Holds[Index];
		const FType* Type = Types.Find(Hold.Type);
		const float MaxHoldTime = Type ? Type->Settings.MaxHoldTime : 0.0f;
		if ((Hold.bHasOwner && !Hold.Owner.IsValid()) || (MaxHoldTime > 0.0f && Now - Hold.Since >= MaxHoldTime))
		{
			ReleaseHold(Index, true, Now);
		}
	}

	for (TPair<FGameplayTag, FType>& Pair : Types)
	{
		FType& Type = Pair.Value;
		Type.CoolingUntil.RemoveAll([Now](double Until) { return Until <= Now; });

		const int32 Max = FMath::Max(Type.Settings.Max, 0);
		if (Type.Settings.RechargeInterval <= 0.0f || Type.Count - Type.PendingRemovals >= Max)
		{
			Type.NextRecharge = 0.0;
			continue;
		}

		// Timed from when it fell short, so a full pool doesn't bank recharges
		if (Type.NextRecharge <= 0.0)
		{
			Type.NextRecharge = Now + Type.Settings.RechargeInterval;
		}
		while (Type.NextRecharge <= Now && Type.Count - Type.PendingRemovals < Max)
		{
			++Type.Count;
			Type.NextRecharge += Type.Settings.RechargeInterval;
		}
	}
}

void UKzTicketPool::ReleaseHold(int32 Index, bool bCooldown, double Now)
{
	const FHold Hold = Holds[Index];
	Holds.RemoveAt(Index);

	FType* Type = Types.Find(Hold.Type);
	if (!Type)
	{
		return;
	}

	Type->Held -= Hold.Amount;

	// Tickets removed while held go away now
	const int32 Gone = FMath::Min(Type->PendingRemovals, Hold.Amount);
	Type->PendingRemovals -= Gone;
	Type->Count -= Gone;

	if (bCooldown)
	{
		Cool(*Type, Hold.Amount - Gone, Now);
	}
}

void UKzTicketPool::Cool(FType& Type, int32 Count, double Now)
{
	const FFloatInterval& Range = Type.Settings.Cooldown;
	if (Count <= 0 || Range.Max <= 0.0f)
	{
		return;
	}

	const float Common = FMath::FRandRange(Range.Min, Range.Max);
	for (int32 Ticket = 0; Ticket < Count; ++Ticket)
	{
		Type.CoolingUntil.Add(Now + (Type.Settings.bCommonCooldown ? Common : FMath::FRandRange(Range.Min, Range.Max)));
	}
}