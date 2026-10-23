// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/PropertyBag.h"
#include "Core/KzTypeDef.h"
#include "Core/KzVariant.h"
#include "KzDatabase.generated.h"

/** Whether an item is a candidate for a query, or the first rule it breaks. */
enum class EKzDatabaseMatch : uint8
{
	Candidate,
	MissingRequiredTag,
	HasIgnoredTag,
	FailsTagQuery,
	UncoveredTag
};

/**
 * What the asker wants and what is true about it. An item is a candidate when it passes RequireTags, IgnoreTags and TagQuery, and each of its own
 * tags is asked for or true of the asker, so variants for other states never apply. The candidate with the most tags wins; on a tie, the first.
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzDatabaseQuery
{
	GENERATED_BODY()

	/** The item must have all of these, or more specific ones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer RequireTags;

	/** The item must have none of these, nor more specific ones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer IgnoreTags;

	/** What is true about the asker: its state, what it holds. A more specific tag covers an item's tag: State.Injured.Leg covers State.Injured. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer ContextTags;

	/** Advanced logic (A OR B AND C), checked after RequireTags and IgnoreTags. Its tags count as asked for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagQuery TagQuery;

	/** Whether an item with these tags is a candidate. */
	bool Matches(const FGameplayTagContainer& Tags) const { return Evaluate(Tags) == EKzDatabaseMatch::Candidate; }

	/** Whether an item with these tags is a candidate, or the first rule it breaks; for an uncovered tag, which one. */
	EKzDatabaseMatch Evaluate(const FGameplayTagContainer& Tags, FGameplayTag* OutUncoveredTag = nullptr) const;

	/** How specific a candidate is: its tag count, all of them covered. */
	int32 CalculateScore(const FGameplayTagContainer& Tags) const;

	/** Returns true if the query has no constraints */
	bool IsEmpty() const;

private:
	/** Whether the item's tag is asked for (under RequireTags or TagQuery) or true of the asker (ContextTags). */
	bool Covers(const FGameplayTag& Tag) const;
};

/** A single entry in the database. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzDatabaseItem
{
	GENERATED_BODY()

	/** Unique Identifier for this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FName ID = NAME_None;

	/** Semantic tags for querying. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FGameplayTagContainer Tags;

	/** The dynamic storage. */
	UPROPERTY(EditAnywhere, Category = "Data", meta = (FixedType, ShowOnlyInnerProperties))
	FKzVariant Value;

public:
	/**
	 * Ensures the internal storage matches the Definition.
	 * Must be called when creating the item or changing the Def.
	 */
	void SyncType(const FKzTypeDef& TypeDef);

	/** Returns true if the bag has a valid value set */
	bool IsValid() const
	{
		return Value.IsValid();
	}

	template <typename T>
	T GetValue() const
	{
		return Value.Get<T>();
	}

	template <typename T>
	void SetValue(const T& InValue)
	{
		Value.Set<T>(InValue);
	}
};

/**
 * A container acting as a database registry.
 * It defines the Type for all its items.
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzDatabase
{
	GENERATED_BODY()

	/** Defines the type of data stored in this database. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Database", meta = (NoArrays))
	FKzTypeDef Type;

	/** The collection of items. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Database", meta = (TitleProperty = "ID"))
	TArray<FKzDatabaseItem> Items;

	FKzDatabase()
	{
		Type.ValueType = EPropertyBagPropertyType::None;
	}

	/** Configures the Database Schema based on a C++ Type. */
	template <typename T>
	void InitType()
	{
		Type.Init<T>();
		Items.Empty();
	}

	/** Clears all items */
	void Empty()
	{
		Items.Empty();
	}

	/** Returns true if there are no items */
	bool IsEmpty() const
	{
		return Items.IsEmpty();
	}

	/** Removes an item by ID. Returns true if removed. */
	bool RemoveItem(FName ID)
	{
		return Items.RemoveAll([&ID](const FKzDatabaseItem& Item) { return Item.ID == ID; }) > 0;
	}

	/**
	 * Adds or Updates an item with type safety checks.
	 * Returns nullptr if the provided type T does not fit the Database Definition.
	 */
	template <typename T>
	FKzDatabaseItem* AddOrUpdateItem(FName ID, const FGameplayTagContainer& Tags, const T& Value)
	{
		// The variant's own rule, the one reads go through: classes by inheritance, everything else exactly.
		FKzVariant NewValue = FKzVariant::Make<T>(Value);
		if (!ensureMsgf(NewValue.MatchesType(Type), TEXT("FKzDatabase::AddOrUpdateItem: the value's type doesn't fit the database's for '%s'."), *ID.ToString()))
		{
			return nullptr;
		}

		FKzDatabaseItem* Item = FindItem(ID);
		if (!Item)
		{
			Item = &Items.AddDefaulted_GetRef();
			Item->ID = ID;
		}

		Item->Tags = Tags;
		Item->Value = MoveTemp(NewValue);
		return Item;
	}

	/** The soft values of the items, added to OutPaths once each. */
	void GetSoftValuePaths(TArray<FSoftObjectPath>& OutPaths) const;

	/** Finds an item by ID */
	const FKzDatabaseItem* FindItem(FName ID) const;
	FKzDatabaseItem* FindItem(FName ID);

	/**
	 * Finds all items matching the query.
	 * Results are sorted by score descending, ties in list order.
	 * @return Number of items found.
	 */
	int32 QueryItems(const FKzDatabaseQuery& Query, TArray<const FKzDatabaseItem*>& OutItems) const;

	/**
	 * Finds the single best match for the query (highest score, ties in list order), and its score if asked.
	 * O(N) complexity and 0 heap allocations. Faster than QueryItems for single lookups.
	 */
	const FKzDatabaseItem* FindBestMatch(const FKzDatabaseQuery& Query, int32* OutScore = nullptr) const;

	/**
	 * Templated helper to find the best match and extract its ID and typed Value directly.
	 * @return True if a match was found and the value type is compatible.
	 */
	template <typename T>
	bool FindBestMatch(const FKzDatabaseQuery& Query, FName& OutID, T& OutValue) const
	{
		const FKzDatabaseItem* BestItem = FindBestMatch(Query);
		if (BestItem && BestItem->Value.Is<T>())
		{
			OutID = BestItem->ID;
			OutValue = BestItem->GetValue<T>();
			return true;
		}
		return false;
	}
};