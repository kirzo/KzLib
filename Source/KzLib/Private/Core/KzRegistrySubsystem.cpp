// Copyright 2026 kirzo

#include "Core/KzRegistrySubsystem.h"
#include "Engine/World.h"

UKzRegistrySubsystem* UKzRegistrySubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UKzRegistrySubsystem>() : nullptr;
}

void UKzRegistrySubsystem::Register(UObject* Item, const UObject* Group)
{
	if (!Item) return;

	const TObjectKey<UObject> ItemKey(Item);
	TArray<FEntry>& Entries = Buckets.FindOrAdd(TObjectKey<UClass>(Item->GetClass()));

	if (FEntry* Entry = Entries.FindByPredicate([&ItemKey](const FEntry& E) { return E.Item == ItemKey; }))
	{
		Entry->Group = Group;
	}
	else
	{
		Entries.Add({ ItemKey, Group });
	}
}

void UKzRegistrySubsystem::Unregister(const UObject* Item)
{
	if (!Item) return;

	if (TArray<FEntry>* Entries = Buckets.Find(TObjectKey<UClass>(Item->GetClass())))
	{
		const TObjectKey<UObject> ItemKey(Item);
		Entries->RemoveAllSwap([&ItemKey](const FEntry& E) { return E.Item == ItemKey; });
	}
}
