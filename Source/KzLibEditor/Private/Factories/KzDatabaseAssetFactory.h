// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "KzDatabaseAssetFactory.generated.h"

UCLASS()
class UKzDatabaseAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UKzDatabaseAssetFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End of UFactory interface
};