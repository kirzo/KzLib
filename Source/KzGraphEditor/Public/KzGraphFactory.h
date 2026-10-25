// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "KzGraphFactory.generated.h"

/** Base factory of graph assets: a domain's factory only sets SupportedClass. New assets start with their default nodes. */
UCLASS(Abstract)
class KZGRAPHEDITOR_API UKzGraphFactory : public UFactory
{
	GENERATED_BODY()

public:
	UKzGraphFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};