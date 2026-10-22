// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/KzDatabase.h"
#include "KzDatabaseAsset.generated.h"

/**
 * A Data Asset that wraps an FKzDatabase, adding inheritance from a parent table.
 * Only contains items of a single specific type defined by the internal Database schema.
 */
UCLASS(BlueprintType, Const)
class KZLIB_API UKzDatabaseAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Optional parent database to inherit items from.
	 * The parent's item wins only with a better score; on a tie, this table's.
	 * Useful for creating base assets (e.g., Human Animations) and extending them (e.g., Knight Animations).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database")
	TObjectPtr<UKzDatabaseAsset> ParentDatabase;

	/** The actual database storage containing the items and defining the schema type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database", meta = (HideItems))
	FKzDatabase Database;

	/** The type of the values stored in this table. */
	FKzTypeDef GetDataType() const
	{
		return Database.Type;
	}

	/**
	 * Finds the best matching item for the given query, across the parent hierarchy, and its score if asked.
	 * @param Query What is asked for and what is true about the asker.
	 * @return A pointer to the best matching item, or nullptr if no match was found.
	 */
	const FKzDatabaseItem* ResolveMatch(const FKzDatabaseQuery& Query, int32* OutScore = nullptr) const;

	/**
	 * Templated helper to find the best match and extract its typed value directly.
	 * @return True if a match was found and the value type is compatible.
	 */
	template <typename T>
	bool ResolveMatch(const FKzDatabaseQuery& Query, T& OutValue) const
	{
		const FKzDatabaseItem* BestItem = ResolveMatch(Query);
		if (BestItem && BestItem->Value.Is<T>())
		{
			OutValue = BestItem->GetValue<T>();
			return true;
		}
		return false;
	}

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
#if WITH_EDITOR
	/** Checks if assigning the given parent would create a circular dependency loop. */
	bool HasCircularDependency(const UKzDatabaseAsset* PotentialParent) const;
#endif
};