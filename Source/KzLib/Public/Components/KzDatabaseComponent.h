// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/KzDatabaseAsset.h"
#include "KzDatabaseComponent.generated.h"

/**
 * Acts as the central data registry for an Actor.
 * Assign Database Assets here, and the component routes queries to the correct database in O(1) time based on the requested data type.
 */
UCLASS(ClassGroup = (Database), meta = (BlueprintSpawnableComponent))
class KZLIB_API UKzDatabaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzDatabaseComponent();

	/** 
	 * The list of databases assigned to this actor.
	 * DO NOT assign multiple databases that resolve the same data type.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database")
	TArray<TObjectPtr<UKzDatabaseAsset>> Databases;

	/**
	 * Queries the component for a specific type of data.
	 * @param Query The tags and rules to find the best match.
	 * @param OutValue The resolved value, if successful.
	 * @return True if a matching database was found AND it successfully resolved the query.
	 */
	template <typename T>
	bool Resolve(const FKzDatabaseQuery& Query, T& OutValue) const
	{
		// 1. Generate the ParamDef locally to use as the Map Key
		FKzParamDef SearchType = FKzParamDef::Make<T>();

		// 2. Find the correct Database Asset for this type
		if (const TObjectPtr<UKzDatabaseAsset>* FoundAsset = DatabaseMap.Find(SearchType))
		{
			if (*FoundAsset)
			{
				// 3. Delegate the resolution (scoring & inheritance) to the Asset
				return (*FoundAsset)->ResolveMatch<T>(Query, OutValue);
			}
		}

		return false;
	}

	/**
	 * Returns the Database Asset associated with a specific parameter type, if any.
	 * Used internally by the Custom Thunk nodes to route queries dynamically.
	 */
	UKzDatabaseAsset* GetDatabaseAsset(const FKzParamDef& SearchType) const
	{
		if (const TObjectPtr<UKzDatabaseAsset>* FoundAsset = DatabaseMap.Find(SearchType))
		{
			return *FoundAsset;
		}
		return nullptr;
	}

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/**
	 * Backend map used for runtime lookups.
	 * Built automatically during BeginPlay from the frontend 'Databases' array.
	 */
	UPROPERTY(Transient)
	TMap<FKzParamDef, TObjectPtr<UKzDatabaseAsset>> DatabaseMap;
};