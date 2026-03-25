// Copyright 2026 kirzo

#include "Factories/KzDatabaseAssetFactory.h"
#include "Core/KzDatabaseAsset.h"

UKzDatabaseAssetFactory::UKzDatabaseAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzDatabaseAsset::StaticClass();
}

UObject* UKzDatabaseAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzDatabaseAsset>(InParent, Class, Name, Flags | RF_Transactional);
}