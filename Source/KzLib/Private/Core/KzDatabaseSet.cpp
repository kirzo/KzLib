// Copyright 2026 kirzo

#include "Core/KzDatabaseSet.h"

const FKzDatabaseItem* FKzDatabaseSet::ResolveItem(const FKzTypeDef& Type, const FKzDatabaseQuery& Query, int32* OutScore) const
{
	// A walk over a handful of tables beats hashing a type, and works before BeginPlay and in editor previews.
	const FKzDatabaseItem* BestItem = nullptr;
	int32 BestScore = -1;
	auto Consider = [&BestItem, &BestScore](const FKzDatabaseItem* Item, int32 Score)
	{
		if (Item && Score > BestScore)
		{
			BestItem = Item;
			BestScore = Score;
		}
	};

	for (const FKzDatabase& Table : Tables)
	{
		if (Table.Type == Type)
		{
			int32 Score = -1;
			const FKzDatabaseItem* Item = Table.FindBestMatch(Query, &Score);
			Consider(Item, Score);
		}
	}

	for (const UKzDatabaseAsset* Asset : Databases)
	{
		if (Asset && Asset->Database.Type == Type)
		{
			int32 Score = -1;
			const FKzDatabaseItem* Item = Asset->ResolveMatch(Query, &Score);
			Consider(Item, Score);
		}
	}

	if (OutScore)
	{
		*OutScore = BestScore;
	}
	return BestItem;
}

const FKzDatabaseItem* FKzDatabaseSet::Resolve(const FKzTypeDef& Type, const FKzDatabaseQuery& Query, const FKzDatabaseSet* ObjectTables, const FGameplayTagContainer& ObjectTags, const FKzDatabaseSet* UserTables, const FGameplayTagContainer& UserTags, bool* bOutFromUser)
{
	FKzDatabaseQuery ContextQuery = Query;
	ContextQuery.ContextTags.AppendTags(ObjectTags);
	ContextQuery.ContextTags.AppendTags(UserTags);

	int32 ObjectScore = -1;
	const FKzDatabaseItem* ObjectItem = ObjectTables ? ObjectTables->ResolveItem(Type, ContextQuery, &ObjectScore) : nullptr;
	int32 UserScore = -1;
	const FKzDatabaseItem* UserItem = UserTables ? UserTables->ResolveItem(Type, ContextQuery, &UserScore) : nullptr;

	// The user overrides the object only with a more specific row.
	const bool bFromUser = UserItem && UserScore > ObjectScore;
	if (bOutFromUser)
	{
		*bOutFromUser = bFromUser;
	}
	return bFromUser ? UserItem : ObjectItem;
}

void FKzDatabaseSet::GetSoftValuePaths(TArray<FSoftObjectPath>& OutPaths) const
{
	for (const FKzDatabase& Table : Tables)
	{
		Table.GetSoftValuePaths(OutPaths);
	}

	for (const UKzDatabaseAsset* Asset : Databases)
	{
		if (Asset)
		{
			Asset->GetSoftValuePaths(OutPaths);
		}
	}
}