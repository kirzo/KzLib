// Copyright 2026 kirzo

#include "Core/KzDatabase.h"
#include "Algo/AnyOf.h"

EKzDatabaseMatch FKzDatabaseQuery::Evaluate(const FGameplayTagContainer& Tags, FGameplayTag* OutUncoveredTag) const
{
	if (!RequireTags.IsEmpty() && !Tags.HasAll(RequireTags))
	{
		return EKzDatabaseMatch::MissingRequiredTag;
	}

	if (!IgnoreTags.IsEmpty() && Tags.HasAny(IgnoreTags))
	{
		return EKzDatabaseMatch::HasIgnoredTag;
	}

	if (!TagQuery.IsEmpty() && !TagQuery.Matches(Tags))
	{
		return EKzDatabaseMatch::FailsTagQuery;
	}

	for (const FGameplayTag& Tag : Tags)
	{
		if (!Covers(Tag))
		{
			if (OutUncoveredTag)
			{
				*OutUncoveredTag = Tag;
			}
			return EKzDatabaseMatch::UncoveredTag;
		}
	}
	return EKzDatabaseMatch::Candidate;
}

int32 FKzDatabaseQuery::CalculateScore(const FGameplayTagContainer& Tags) const
{
	return Tags.Num();
}

bool FKzDatabaseQuery::IsEmpty() const
{
	return RequireTags.IsEmpty() && IgnoreTags.IsEmpty() && ContextTags.IsEmpty() && TagQuery.IsEmpty();
}

bool FKzDatabaseQuery::Covers(const FGameplayTag& Tag) const
{
	return Tag.MatchesAny(RequireTags) || ContextTags.HasTag(Tag) || Algo::AnyOf(TagQuery.GetGameplayTagArray(), [&Tag](const FGameplayTag& QueryTag) { return Tag.MatchesTag(QueryTag); });
}

void FKzDatabaseItem::SyncType(const FKzTypeDef& TypeDef)
{
	if (!Value.MatchesType(TypeDef))
	{
		Value.SetType(TypeDef.ValueType, TypeDef.ValueTypeObject);
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
			ScoredResults.Add({ &Item, Query.CalculateScore(Item.Tags) });
		}
	}

	ScoredResults.StableSort();

	OutItems.Reserve(ScoredResults.Num());
	for (const FScoredItem& Entry : ScoredResults)
	{
		OutItems.Add(Entry.Item);
	}

	return OutItems.Num();
}

const FKzDatabaseItem* FKzDatabase::FindBestMatch(const FKzDatabaseQuery& Query, int32* OutScore) const
{
	const FKzDatabaseItem* BestItem = nullptr;
	int32 BestScore = -1;

	for (const FKzDatabaseItem& Item : Items)
	{
		if (Query.Matches(Item.Tags))
		{
			const int32 Score = Query.CalculateScore(Item.Tags);
			if (Score > BestScore)
			{
				BestScore = Score;
				BestItem = &Item;
			}
		}
	}

	if (OutScore)
	{
		*OutScore = BestScore;
	}
	return BestItem;
}