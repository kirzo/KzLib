// Copyright 2026 kirzo

#include "Components/KzDatabaseComponent.h"
#include "Engine/AssetManager.h"
#include "GameplayTagAssetInterface.h"

namespace KzDatabaseComponent
{
	void AppendGameplayTags(const AActor* Actor, FGameplayTagContainer& OutTags)
	{
		// Implementations may reset the container they fill.
		if (const IGameplayTagAssetInterface* TagOwner = Cast<IGameplayTagAssetInterface>(Actor))
		{
			FGameplayTagContainer Owned;
			TagOwner->GetOwnedGameplayTags(Owned);
			OutTags.AppendTags(Owned);
		}
	}
}

UKzDatabaseComponent::UKzDatabaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const FKzDatabaseItem* UKzDatabaseComponent::ResolveItem(const FKzTypeDef& Type, const FKzDatabaseQuery& Query) const
{
	FKzDatabaseQuery ContextQuery = Query;
	AppendContextTags(ContextQuery.ContextTags);
	return Databases.ResolveItem(Type, ContextQuery);
}

const FKzDatabaseItem* UKzDatabaseComponent::ResolveItem(const FKzTypeDef& Type, const FKzDatabaseQuery& Query, const FKzDatabaseSet* ObjectTables, const FGameplayTagContainer& ObjectTags, const AActor* User)
{
	FKzDatabaseQuery ContextQuery = Query;
	ContextQuery.ContextTags.AppendTags(ObjectTags);
	AppendContextTags(User, ContextQuery.ContextTags);

	int32 ObjectScore = -1;
	const FKzDatabaseItem* ObjectItem = ObjectTables ? ObjectTables->ResolveItem(Type, ContextQuery, &ObjectScore) : nullptr;

	// The user overrides the object only with a more specific row.
	const UKzDatabaseComponent* UserComponent = User ? User->FindComponentByClass<UKzDatabaseComponent>() : nullptr;
	int32 UserScore = -1;
	const FKzDatabaseItem* UserItem = UserComponent ? UserComponent->Databases.ResolveItem(Type, ContextQuery, &UserScore) : nullptr;
	return UserItem && UserScore > ObjectScore ? UserItem : ObjectItem;
}

void UKzDatabaseComponent::AppendContextTags(FGameplayTagContainer& OutTags) const
{
	OutTags.AppendTags(Tags);
	KzDatabaseComponent::AppendGameplayTags(GetOwner(), OutTags);
}

void UKzDatabaseComponent::AppendContextTags(const AActor* Actor, FGameplayTagContainer& OutTags)
{
	if (const UKzDatabaseComponent* Component = Actor ? Actor->FindComponentByClass<UKzDatabaseComponent>() : nullptr)
	{
		Component->AppendContextTags(OutTags);
	}
	else
	{
		KzDatabaseComponent::AppendGameplayTags(Actor, OutTags);
	}
}

void UKzDatabaseComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bPreloadValues)
	{
		return;
	}

	TArray<FSoftObjectPath> Paths;
	Databases.GetSoftValuePaths(Paths);
	if (!Paths.IsEmpty())
	{
		PreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(Paths));
	}
}

void UKzDatabaseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
		PreloadHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}