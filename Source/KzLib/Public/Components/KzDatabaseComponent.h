// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Core/KzDatabaseSet.h"
#include "KzDatabaseComponent.generated.h"

struct FStreamableHandle;

/**
 * The actor's tables, so a system can ask the actor without knowing which actor it is. What the actor is and its state (Tags, and its gameplay tags
 * through IGameplayTagAssetInterface) join the context of every query it takes part in, so they pick the variants.
 * A query about an object made by a user goes to both: the object's tables answer, and the user's override them only with a more specific row.
 */
UCLASS(ClassGroup = (Database), meta = (BlueprintSpawnableComponent))
class KZLIB_API UKzDatabaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzDatabaseComponent();

	/** The actor's tables; several may hold the same type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database", meta = (ShowOnlyInnerProperties))
	FKzDatabaseSet Databases;

	/** What the actor is, for the queries it takes part in, such as Item.Heavy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Database")
	FGameplayTagContainer Tags;

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

	/** The best item for the query across the actor's tables, the actor's tags joining the context. */
	const FKzDatabaseItem* ResolveItem(const FKzTypeDef& Type, const FKzDatabaseQuery& Query) const;

	/** A query about an object made by a user: both's tags join the context, the object's tables answer, and the user's override them only with a more specific row. */
	static const FKzDatabaseItem* ResolveItem(const FKzTypeDef& Type, const FKzDatabaseQuery& Query, const FKzDatabaseSet* ObjectTables, const FGameplayTagContainer& ObjectTags, const AActor* User);

	/** Adds what the actor is and its state: Tags here, and the owner's gameplay tags. */
	void AppendContextTags(FGameplayTagContainer& OutTags) const;

	/** The same for any actor: through its database component when it has one, else just its gameplay tags. */
	static void AppendContextTags(const AActor* Actor, FGameplayTagContainer& OutTags);

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

	/** Resolves a query about an object made by a user; Default when the query asks nothing or neither answers. */
	template <typename T>
	static T ResolveFor(const FKzDatabaseSet* ObjectTables, const FGameplayTagContainer& ObjectTags, const AActor* User, const FKzDatabaseQuery& Query, const T& Default)
	{
		const FKzDatabaseItem* Item = Query.IsEmpty() ? nullptr : ResolveItem(FKzTypeDef::Make<T>(), Query, ObjectTables, ObjectTags, User);
		return Item && Item->Value.Is<T>() ? Item->GetValue<T>() : Default;
	}

	/** The same with an actor as the object: its database component's tables, and what it is. */
	template <typename T>
	static T ResolveFor(const AActor* Object, const AActor* User, const FKzDatabaseQuery& Query, const T& Default)
	{
		const UKzDatabaseComponent* ObjectComponent = Object ? Object->FindComponentByClass<UKzDatabaseComponent>() : nullptr;
		FGameplayTagContainer ObjectTags;
		AppendContextTags(Object, ObjectTags);
		return ResolveFor<T>(ObjectComponent ? &ObjectComponent->Databases : nullptr, ObjectTags, User, Query, Default);
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Keeps the preloaded values in memory while the actor plays. */
	TSharedPtr<FStreamableHandle> PreloadHandle;
};