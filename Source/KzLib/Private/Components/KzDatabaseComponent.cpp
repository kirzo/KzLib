// Copyright 2026 kirzo

#include "Components/KzDatabaseComponent.h"
#include "Engine/AssetManager.h"
#include "GameplayTagAssetInterface.h"

UKzDatabaseComponent::UKzDatabaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const FKzDatabaseItem* UKzDatabaseComponent::ResolveItem(const FKzTypeDef& Type, const FKzDatabaseQuery& Query) const
{
	// The actor's state is the context it asks in, on top of whatever the query brings.
	FGameplayTagContainer OwnerTags;
	if (const IGameplayTagAssetInterface* TagOwner = Cast<IGameplayTagAssetInterface>(GetOwner()))
	{
		TagOwner->GetOwnedGameplayTags(OwnerTags);
	}
	FKzDatabaseQuery OwnerQuery;
	if (!OwnerTags.IsEmpty())
	{
		OwnerQuery = Query;
		OwnerQuery.ContextTags.AppendTags(OwnerTags);
	}
	const FKzDatabaseQuery& EffectiveQuery = OwnerTags.IsEmpty() ? Query : OwnerQuery;

	// A walk over a handful of tables beats hashing a type, and works before BeginPlay and in editor previews.
	const FKzDatabaseItem* BestItem = nullptr;
	int32 BestScore = -1;
	for (const UKzDatabaseAsset* Asset : Databases)
	{
		if (!Asset || Asset->Database.Type != Type)
		{
			continue;
		}

		int32 Score = -1;
		const FKzDatabaseItem* Item = Asset->ResolveMatch(EffectiveQuery, &Score);
		if (Item && Score > BestScore)
		{
			BestItem = Item;
			BestScore = Score;
		}
	}
	return BestItem;
}

void UKzDatabaseComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bPreloadValues)
	{
		return;
	}

	TArray<FSoftObjectPath> Paths;
	for (const UKzDatabaseAsset* Asset : Databases)
	{
		if (Asset)
		{
			Asset->GetSoftValuePaths(Paths);
		}
	}

	if (!Paths.IsEmpty())
	{
		PreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(Paths));
	}
}

void UKzDatabaseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
		PreloadHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}