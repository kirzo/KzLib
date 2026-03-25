// Copyright 2026 kirzo

#include "Components/KzDatabaseComponent.h"

#if WITH_EDITOR
#include "Misc/MessageDialog.h"
#endif

UKzDatabaseComponent::UKzDatabaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKzDatabaseComponent::BeginPlay()
{
	Super::BeginPlay();

	// Autowire the Backend Map for O(1) lookups
	DatabaseMap.Empty(Databases.Num());

	for (UKzDatabaseAsset* Asset : Databases)
	{
		if (Asset && Asset->Database.Type.IsValid())
		{
			// The key is the schema type, the value is the asset
			DatabaseMap.Add(Asset->GetDataType(), Asset);
		}
	}
}

#if WITH_EDITOR
void UKzDatabaseComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Validate that designers don't assign multiple databases of the exact same type
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UKzDatabaseComponent, Databases))
	{
		TSet<FKzParamDef> SeenTypes;
		bool bFoundDuplicate = false;

		for (UKzDatabaseAsset* Asset : Databases)
		{
			if (!Asset) continue;

			FKzParamDef AssetType = Asset->GetDataType();
			if (AssetType.IsValid())
			{
				if (SeenTypes.Contains(AssetType))
				{
					bFoundDuplicate = true;
					break;
				}
				SeenTypes.Add(AssetType);
			}
		}

		if (bFoundDuplicate)
		{
			FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Warning: You have assigned multiple Database Assets that resolve the exact same Data Type.\n\nThe UKzDatabaseComponent expects a maximum of ONE Database per type. The system will only use the last one registered at runtime."));
		}
	}
}
#endif