// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "KzRegistrySubsystem.generated.h"

/**
 * A registry of world objects, queried by class (subclasses included) and optionally by group, such as the spline a marker sits on.
 * It also exists in editor worlds: actors that register in PostRegisterAllComponents and unregister in UnregisterAllComponents
 * are found in the editor, in PIE and across level streaming.
 */
UCLASS()
class KZLIB_API UKzRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UKzRegistrySubsystem* Get(const UObject* WorldContextObject);

	/** Registers Item under its own class. Registering it again only moves it to the new group. */
	void Register(UObject* Item, const UObject* Group = nullptr);

	void Unregister(const UObject* Item);

	/** Calls Func(T*) for every live item of class T or a subclass in Group; a null Group matches every group. Func must not register or unregister. */
	template <typename T, typename FuncType>
	void ForEach(const UObject* Group, FuncType&& Func)
	{
		const TObjectKey<UObject> GroupKey(Group);

		for (auto BucketIt = Buckets.CreateIterator(); BucketIt; ++BucketIt)
		{
			const UClass* Class = BucketIt.Key().ResolveObjectPtr();
			if (!Class)
			{
				BucketIt.RemoveCurrent();
				continue;
			}

			if (!Class->IsChildOf(T::StaticClass()))
			{
				continue;
			}

			// Backwards, so RemoveAtSwap only pulls in entries already visited.
			TArray<FEntry>& Entries = BucketIt.Value();
			for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
			{
				UObject* Item = Entries[Index].Item.ResolveObjectPtr();
				if (!Item)
				{
					Entries.RemoveAtSwap(Index);
				}
				else if (!Group || Entries[Index].Group == GroupKey)
				{
					Func(static_cast<T*>(Item));
				}
			}
		}
	}

	template <typename T, typename FuncType>
	void ForEach(FuncType&& Func)
	{
		ForEach<T>(nullptr, Forward<FuncType>(Func));
	}

private:
	struct FEntry
	{
		TObjectKey<UObject> Item;
		TObjectKey<UObject> Group;
	};

	/** Keyed by each item's own class; a query walks every class that derives from the one it asks for. */
	TMap<TObjectKey<UClass>, TArray<FEntry>> Buckets;
};
