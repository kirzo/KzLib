// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Math/Interval.h"
#include "UObject/Object.h"
#include "KzTicketPool.generated.h"

class UKzTicketPool;

UENUM(BlueprintType)
enum class EKzTicketState : uint8
{
	/** Ready to be acquired. */
	Free,

	/** Acquired, until whoever holds it releases it. */
	Held,

	/** Released, and back to Free once its cooldown is over. */
	Cooling
};

/** How one type of ticket of a pool behaves. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzTicketTypeSettings
{
	GENERATED_BODY()

	/** How many there are when the pool is created or reset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (ClampMin = "0"))
	int32 Initial = 1;

	/** How many there can be at most, by recharging or by Add Tickets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (ClampMin = "0"))
	int32 Max = 1;

	/** One more every this long while there are fewer than Max, so a pool can start with few and fill up. 0 never adds any. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (ClampMin = "0", Units = "Seconds"))
	float RechargeInterval = 0.0f;

	/** How long a released ticket takes to be free again, at random in this range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets")
	FFloatInterval Cooldown = FFloatInterval(0.0f, 0.0f);

	/** Tickets released together cool down for the same time; off, each draws its own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets")
	bool bCommonCooldown = false;

	/** A handle held longer than this is released by the pool, in case its holder never does. 0 has no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (ClampMin = "0", Units = "Seconds"))
	float MaxHoldTime = 0.0f;
};

/** The ticket types of a pool, each with its settings. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzTicketPoolSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tickets", meta = (Categories = "Ticket"))
	TMap<FGameplayTag, FKzTicketTypeSettings> Types;
};

/** Tickets acquired from a pool, to release when done. Empty when the acquisition failed. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzTicketHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id != 0; }
	UKzTicketPool* GetPool() const { return Pool.Get(); }

private:
	friend class UKzTicketPool;

	UPROPERTY()
	TWeakObjectPtr<UKzTicketPool> Pool;

	UPROPERTY()
	int32 Id = 0;
};

/**
 * Counted permissions to do something, by type: how many may attack at once, or speak, or anything else that has to take turns. Whoever
 * wants to act acquires tickets and releases them when done; released tickets cool down before they are free again. A holder that is
 * destroyed, or that holds longer than its type allows, loses its tickets as if it released them.
 * It never ticks: cooldowns, recharges and lost holders are brought up to date whenever it is asked, with its outer's world time, or the
 * platform's outside a world. Server only.
 */
UCLASS(BlueprintType)
class KZLIB_API UKzTicketPool : public UObject
{
	GENERATED_BODY()

public:
	/** A pool with these types and their Initial tickets, owned by Outer, whose world gives it the time. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets", meta = (DefaultToSelf = "Outer"))
	static UKzTicketPool* CreateTicketPool(UObject* Outer, const FKzTicketPoolSettings& Settings);

	/** New settings while in use: types that are gone are dropped with their handles, new ones start with Initial, the rest keep their tickets within Max. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void ApplySettings(const FKzTicketPoolSettings& NewSettings);

	const FKzTicketPoolSettings& GetSettings() const { return Settings; }

	/** Back to the Initial tickets of each type, all free. Every handle is lost. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void Reset();

	UFUNCTION(BlueprintPure, Category = "KzLib|Tickets")
	bool CanAcquire(UPARAM(meta = (Categories = "Ticket")) FGameplayTag Type, int32 Amount = 1);

	/** Amount free tickets of a type for Owner, or an empty handle if there are not as many. They come back if Owner is destroyed; without an Owner, only when released. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets", meta = (DefaultToSelf = "Owner"))
	FKzTicketHandle TryAcquire(UPARAM(meta = (Categories = "Ticket")) FGameplayTag Type, int32 Amount, UObject* Owner);

	/** Gives the handle's tickets back and empties it. Without cooldown they are free at once, such as for an action called off before it started. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void Release(UPARAM(ref) FKzTicketHandle& Handle, bool bCooldown = true);

	/** False once the handle's tickets are back, whether released, lost with their holder, held too long or reset. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Tickets")
	bool IsHeld(const FKzTicketHandle& Handle);

	/** More tickets of a type, up to its Max. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void AddTickets(UPARAM(meta = (Categories = "Ticket")) FGameplayTag Type, int32 Count = 1);

	/** Fewer tickets of a type: free ones first, then cooling ones; held ones go as they come back. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void RemoveTickets(UPARAM(meta = (Categories = "Ticket")) FGameplayTag Type, int32 Count = 1);

	/** Every free ticket of a type starts cooling down, so none is free for a while. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	void StartCooldown(UPARAM(meta = (Categories = "Ticket")) FGameplayTag Type);

	UFUNCTION(BlueprintPure, Category = "KzLib|Tickets")
	int32 GetCount(UPARAM(meta = (Categories = "Ticket")) FGameplayTag Type, EKzTicketState State);

	/** How long each cooling ticket of a type has left, soonest first. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets")
	TArray<float> GetCooldownsLeft(UPARAM(meta = (Categories = "Ticket")) FGameplayTag Type);

	/** Releases a handle through its own pool, for whoever kept the handle and not the pool. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Tickets", meta = (DisplayName = "Release Tickets"))
	static void ReleaseTickets(UPARAM(ref) FKzTicketHandle& Handle, bool bCooldown = true);

	virtual UWorld* GetWorld() const override;

private:
	struct FType
	{
		FKzTicketTypeSettings Settings;

		/** Every ticket there is, held ones included. */
		int32 Count = 0;

		int32 Held = 0;

		/** When each cooling ticket is free again. */
		TArray<double> CoolingUntil;

		/** Held tickets that go away when they come back, removed while held. */
		int32 PendingRemovals = 0;

		/** When the next recharge adds one; 0 when not recharging. */
		double NextRecharge = 0.0;

		int32 GetFree() const { return Count - Held - CoolingUntil.Num(); }
	};

	struct FHold
	{
		int32 Id = 0;
		FGameplayTag Type;
		int32 Amount = 0;
		TWeakObjectPtr<UObject> Owner;
		bool bHasOwner = false;
		double Since = 0.0;
	};

	double GetTime() const;

	/** Brings lost holders, cooldowns and recharges up to now. */
	void Refresh();

	void ReleaseHold(int32 Index, bool bCooldown, double Now);
	void Cool(FType& Type, int32 Count, double Now);

	FKzTicketPoolSettings Settings;
	TMap<FGameplayTag, FType> Types;
	TArray<FHold> Holds;
	int32 NextId = 1;
};