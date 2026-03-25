// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/KzDatabase.h"
#include "KzDatabaseLibrary.generated.h"

class UKzDatabaseAsset;
class UKzDatabaseComponent;

UCLASS()
class KZLIB_API UKzDatabaseLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// --- Management ---

	/** Adds a new item or updates an existing one. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Database", CustomThunk, meta = (CustomStructureParam = "Value", AutoCreateRefTerm = "Tags"))
	static bool AddDatabaseItem(UPARAM(ref) FKzDatabase& Database, FName ID, FGameplayTagContainer Tags, const int32& Value);

	/** Sets the value of an existing item. Returns false if ID not found. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Database", CustomThunk, meta = (CustomStructureParam = "Value"))
	static bool SetDatabaseItemValue(UPARAM(ref) FKzDatabase& Database, FName ID, const int32& Value);

	/** Retrieves the value of an item. Returns false if ID not found or type mismatch. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Database", CustomThunk, meta = (CustomStructureParam = "Value"))
	static bool GetDatabaseItemValue(const FKzDatabase& Database, FName ID, int32& Value);

	/** Removes an item by ID. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Database")
	static bool RemoveDatabaseItem(UPARAM(ref) FKzDatabase& Database, FName ID);

	/** Clears the database. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Database")
	static void EmptyDatabase(UPARAM(ref) FKzDatabase& Database);

	/** Checks if database is empty. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Database")
	static bool IsDatabaseEmpty(const FKzDatabase& Database);

	/** Finds the single best match for the query. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Database", CustomThunk, meta = (CustomStructureParam = "OutValue"))
	static bool FindBestMatch(const FKzDatabase& Database, const FKzDatabaseQuery& Query, FName& OutID, int32& OutValue);

	/**
	 * Internal function used by the EvaluateDatabaseAsset node.
	 * Evaluates the query against the asset and outputs the result in the wildcard out pin.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (BlueprintInternalUseOnly = "true", CustomStructureParam = "OutValue"))
	static bool EvaluateDatabaseAsset(UKzDatabaseAsset* Asset, const FKzDatabaseQuery& Query, int32& OutValue);

	/**
	 * Internal function used by the ResolveDatabaseQuery node.
	 * Asks the Database Component to resolve a query and output the matched item based on the pin type.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (BlueprintInternalUseOnly = "true", CustomStructureParam = "OutValue"))
	static bool ResolveDatabaseQuery(UKzDatabaseComponent* Component, const FKzDatabaseQuery& Query, int32& OutValue);

private:
	// --- Internal Thunk Declarations ---

	DECLARE_FUNCTION(execAddDatabaseItem);
	DECLARE_FUNCTION(execSetDatabaseItemValue);
	DECLARE_FUNCTION(execGetDatabaseItemValue);
	DECLARE_FUNCTION(execFindBestMatch);
	DECLARE_FUNCTION(execEvaluateDatabaseAsset);
	DECLARE_FUNCTION(execResolveDatabaseQuery);
};