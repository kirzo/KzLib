// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KzRegistrySubsystem.generated.h"

/** Helper struct to wrap the array so we can store it in a TMap */
struct FKzObjectBucket
{
	/** We use Weak Pointers for safety. If an object is destroyed, it becomes stale automatically. */
	TArray<TWeakObjectPtr<UObject>> Objects;
};

/** A central registry for any type of object in the world. */
UCLASS()
class KZLIB_API UKzRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Registration API ---

	/** Registers an item under a specific category (Class). */
	template <typename T>
	void RegisterItem(T* Item)
	{
		if (!Item) return;
		UClass* Key = T::StaticClass();

		FKzObjectBucket& Bucket = RegistryMap.FindOrAdd(Key);
		if (!Bucket.Objects.Contains(Item))
		{
			Bucket.Objects.Add(Item);
		}
	}

	/** Unregisters an item. */
	template <typename T>
	void UnregisterItem(T* Item)
	{
		if (!Item) return;
		UClass* Key = T::StaticClass();

		if (FKzObjectBucket* Bucket = RegistryMap.Find(Key))
		{
			Bucket->Objects.RemoveSwap(Item);
		}
	}

	// --- Retrieval API ---

	/** Returns items registered under type T. */
	template <typename T>
	TArray<T*> GetItems()
	{
		TArray<T*> Result;
		UClass* Key = T::StaticClass();

		if (FKzObjectBucket* Bucket = RegistryMap.Find(Key))
		{
			ProcessBucket<T>(*Bucket, Result);
		}
		return Result;
	}

	/** Looks up the bucket using 'RequestedClass', * but returns the objects cast to 'T'. */
	template <typename T>
	TArray<T*> GetItems(TSubclassOf<UObject> RequestedClass)
	{
		TArray<T*> Result;
		if (!RequestedClass) return Result;

		if (FKzObjectBucket* Bucket = RegistryMap.Find(RequestedClass))
		{
			ProcessBucket<T>(*Bucket, Result);
		}
		return Result;
	}

private:
	/** Iterates the bucket, removes stale objects, and casts valid ones to T. */
	template <typename T>
	void ProcessBucket(FKzObjectBucket& Bucket, TArray<T*>& OutResult)
	{
		for (int32 i = 0; i < Bucket.Objects.Num(); ++i)
		{
			if (UObject* Obj = Bucket.Objects[i].Get())
			{
				// Valid object, attempt cast to T
				if (T* CastedObj = Cast<T>(Obj))
				{
					OutResult.Add(CastedObj);
				}
				// Note: If object exists but cast fails, we simply skip it.
				// We don't remove it because it belongs in this bucket, just isn't the type T we asked for now.
			}
			else
			{
				// Object was destroyed (GC'd), remove from registry
				Bucket.Objects.RemoveAtSwap(i);
				--i;
			}
		}
	}

	TMap<UClass*, FKzObjectBucket> RegistryMap;
};