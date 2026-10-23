// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzDatabaseAsset.h"
#include "KzDatabaseSet.generated.h"

/**
 * The tables something owns: its own rows inline, and shared tables as assets, which may inherit. A query goes to every table of the requested type;
 * the most specific row wins, a tie going to the table listed first, inline ones before assets.
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzDatabaseSet
{
	GENERATED_BODY()

	/** Rows of this owner alone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database")
	TArray<FKzDatabase> Tables;

	/** Shared tables, such as a base for every humanoid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database")
	TArray<TObjectPtr<UKzDatabaseAsset>> Databases;

	/** The best item for the query across the tables of the type, and its score if asked. */
	const FKzDatabaseItem* ResolveItem(const FKzTypeDef& Type, const FKzDatabaseQuery& Query, int32* OutScore = nullptr) const;

	/** The soft values the tables can answer with, parents included, added to OutPaths once each. */
	void GetSoftValuePaths(TArray<FSoftObjectPath>& OutPaths) const;
};