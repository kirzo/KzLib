// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Core/KzDatabaseAsset.h"
#include "KzDatabaseComponent.generated.h"

struct FStreamableHandle;

/**
 * The actor's tables, so a system can ask the actor without knowing which actor it is. A query goes to every table of the requested type, and the best
 * item across them wins. The actor's gameplay tags (IGameplayTagAssetInterface) join the query's context: its state picks the variants.
 */
UCLASS(ClassGroup = (Database), meta = (BlueprintSpawnableComponent))
class KZLIB_API UKzDatabaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzDatabaseComponent();

	/** The tables assigned to this actor; several may hold the same type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database")
	TArray<TObjectPtr<UKzDatabaseAsset>> Databases;

	/** Loads the soft values of these tables and their parents in the background when play begins, so a query answers without a synchronous load. */
	UPROPERTY(EditAnywhere, Category = "Database")
	bool bPreloadValues = true;

	/**
	 * Queries the component for a specific type of data.
	 * @param Query The tags and rules to find the best match.
	 * @param OutValue The resolved value, if successful.
	 * @return True if a table of the type resolved the query.
	 */
	template <typename T>
	bool Resolve(const FKzDatabaseQuery& Query, T& OutValue) const
	{
		const FKzDatabaseItem* BestItem = ResolveItem(FKzTypeDef::Make<T>(), Query);
		if (BestItem && BestItem->Value.Is<T>())
		{
			OutValue = BestItem->GetValue<T>();
			return true;
		}
		return false;
	}

	/** The best item for the query across the tables of the type: the highest score, a tie going to the table listed first. */
	const FKzDatabaseItem* ResolveItem(const FKzTypeDef& Type, const FKzDatabaseQuery& Query) const;

	/** Resolves through the actor's database component when it has one and the query asks something; Default when that finds nothing. */
	template <typename T>
	static T ResolveFor(const AActor* Actor, const FKzDatabaseQuery& Query, const T& Default)
	{
		T Value = Default;
		const UKzDatabaseComponent* Component = Actor && !Query.IsEmpty() ? Actor->FindComponentByClass<UKzDatabaseComponent>() : nullptr;
		if (Component)
		{
			Component->Resolve<T>(Query, Value);
		}
		return Value;
	}
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Keeps the preloaded values in memory while the actor plays. */
	TSharedPtr<FStreamableHandle> PreloadHandle;
};