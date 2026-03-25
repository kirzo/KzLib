// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/KzDatabase.h"
#include "KzDatabaseAsset.generated.h"

/**
 * A Data Asset that wraps an FKzDatabase, providing heuristic scoring and hierarchical inheritance.
 * Only contains items of a single specific type defined by the internal Database schema.
 */
UCLASS(BlueprintType, Const)
class KZLIB_API UKzDatabaseAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Optional parent database to inherit items from.
	 * If a query yields a better score in the parent, the parent's item will be returned.
	 * Useful for creating base assets (e.g., Human Animations) and extending them (e.g., Knight Animations).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database")
	TObjectPtr<UKzDatabaseAsset> ParentDatabase;

	/** The actual database storage containing the items and defining the schema type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database")
	FKzDatabase Database;

	/**
	 * Returns the parameter definition (Type) of the data stored in this asset.
	 * Used by container components to route queries efficiently in O(1).
	 */
	FKzParamDef GetDataType() const
	{
		return Database.Type;
	}

	/**
	 * Finds the best matching item for the given query, considering heuristic scoring and the parent hierarchy.
	 * @param Query The query containing required, ignored, and optional tags.
	 * @return A pointer to the best matching item, or nullptr if no match was found.
	 */
	const FKzDatabaseItem* ResolveMatch(const FKzDatabaseQuery& Query) const;

	/**
	 * Templated helper to find the best match and extract its typed value directly.
	 * @return True if a match was found and the value type is compatible.
	 */
	template <typename T>
	bool ResolveMatch(const FKzDatabaseQuery& Query, T& OutValue) const
	{
		if (const FKzDatabaseItem* BestItem = ResolveMatch(Query))
		{
			TValueOrError<T, EPropertyBagResult> Result = BestItem->GetValue<T>();
			if (Result.HasValue())
			{
				OutValue = Result.GetValue();
				return true;
			}
		}
		return false;
	}

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	/** Internal recursive function to find the best match and its score. */
	const FKzDatabaseItem* ResolveBestMatchRecursive(const FKzDatabaseQuery& Query, int32& OutBestScore) const;

private:
#if WITH_EDITOR
	/** Checks if assigning the given parent would create a circular dependency loop. */
	bool HasCircularDependency(const UKzDatabaseAsset* PotentialParent) const;
#endif
};