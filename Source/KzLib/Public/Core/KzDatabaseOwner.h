// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "KzDatabaseOwner.generated.h"

struct FKzDatabaseSet;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UKzDatabaseOwner : public UInterface
{
	GENERATED_BODY()
};

/** Something that owns database tables and says what it is, such as a database component or an item's Database fragment; tools find tables through it. */
class KZLIB_API IKzDatabaseOwner
{
	GENERATED_BODY()

public:
	virtual const FKzDatabaseSet& GetDatabaseSet() const = 0;

	/** What the owner is, for the context of the queries it takes part in. */
	virtual void AppendDatabaseTags(FGameplayTagContainer& OutTags) const = 0;
};