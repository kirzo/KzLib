// Copyright 2026 kirzo

#include "Core/KzDatabase.h"
#include "Algo/Count.h"

bool FKzDatabaseQuery::Matches(const FGameplayTagContainer& Tags) const
{
	// Hard Requirements (ALL)
	if (!RequireTags.IsEmpty() && !Tags.HasAll(RequireTags))
	{
		return false;
	}

	// Exclusions (ANY)
	if (!IgnoreTags.IsEmpty() && Tags.HasAny(IgnoreTags))
	{
		return false;
	}

	// Complex Query
	if (!TagQuery.IsEmpty() && !TagQuery.Matches(Tags))
	{
		return false;
	}

	return true;
}

int32 FKzDatabaseQuery::CalculateScore(const FGameplayTagContainer& Tags) const
{
	if (OptionalTags.IsEmpty())
	{
		return 0;
	}
	// Count how many optional tags are present in the item
  return Algo::CountIf(OptionalTags, [Tags](const FGameplayTag& OptionalTag) { return OptionalTag.MatchesAnyExact(Tags); });
}

bool FKzDatabaseQuery::IsEmpty() const
{
	return RequireTags.IsEmpty() && IgnoreTags.IsEmpty() && OptionalTags.IsEmpty() && TagQuery.IsEmpty();
}

void FKzDatabaseItem::SyncType(const FKzParamDef& Def)
{
	static const FName PropName("Data");
	const FPropertyBagPropertyDesc* ExistingProp = Data.FindPropertyDescByName(PropName);

	// Check if we need to update
	bool bMatches = ExistingProp
		&& ExistingProp->ValueType == Def.ValueType
		&& ExistingProp->ValueTypeObject == Def.ValueTypeObject
		&& ExistingProp->ContainerTypes.GetFirstContainerType() == Def.ContainerType;

	if (bMatches)
	{
		return;
	}

	// Reset and recreate
	Data.Reset();

	if (Def.IsValid())
	{
		Data.AddProperty(PropName, Def.ValueType, Def.ValueTypeObject.Get());
	}
}

const FKzDatabaseItem* FKzDatabase::FindItem(FName ID) const
{
	return Items.FindByPredicate([&ID](const FKzDatabaseItem& Item) { return Item.ID == ID; });
}

FKzDatabaseItem* FKzDatabase::FindItem(FName ID)
{
	return Items.FindByPredicate([&ID](const FKzDatabaseItem& Item) { return Item.ID == ID; });
}

int32 FKzDatabase::QueryItems(const FKzDatabaseQuery& Query, TArray<const FKzDatabaseItem*>& OutItems) const
{
	OutItems.Reset();

	struct FScoredItem
	{
		const FKzDatabaseItem* Item;
		int32 Score;
		bool operator<(const FScoredItem& Other) const { return Score > Other.Score; }
	};
	TArray<FScoredItem> ScoredResults;

	for (const FKzDatabaseItem& Item : Items)
	{
		if (Query.Matches(Item.Tags))
		{
			int32 Score = Query.CalculateScore(Item.Tags);
			ScoredResults.Add({ &Item, Score });
		}
	}

	if (!Query.OptionalTags.IsEmpty())
	{
		ScoredResults.StableSort();
	}

	OutItems.Reserve(ScoredResults.Num());
	for (const FScoredItem& Entry : ScoredResults)
	{
		OutItems.Add(Entry.Item);
	}

	return OutItems.Num();
}

const FKzDatabaseItem* FKzDatabase::FindBestMatch(const FKzDatabaseQuery& Query) const
{
	const FKzDatabaseItem* BestItem = nullptr;
	int32 BestScore = -1;

	for (const FKzDatabaseItem& Item : Items)
	{
		if (Query.Matches(Item.Tags))
		{
			const int32 Score = Query.CalculateScore(Item.Tags);

			// If we found a strictly better score, update the winner
			if (Score > BestScore)
			{
				BestScore = Score;
				BestItem = &Item;
			}
		}
	}

	return BestItem;
}