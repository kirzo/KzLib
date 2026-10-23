// Copyright 2026 kirzo

#include "Components/KzDatabaseComponent.h"
#include "Engine/AssetManager.h"

UKzDatabaseComponent::UKzDatabaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const FKzDatabaseItem* UKzDatabaseComponent::ResolveItem(const FKzTypeDef& Type, const FKzDatabaseQuery& Query) const
{
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
		const FKzDatabaseItem* Item = Asset->ResolveMatch(Query, &Score);
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