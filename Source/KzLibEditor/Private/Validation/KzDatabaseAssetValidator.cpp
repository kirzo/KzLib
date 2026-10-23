// Copyright 2026 kirzo

#include "Validation/KzDatabaseAssetValidator.h"
#include "Validation/KzDatabaseValidation.h"
#include "Core/KzDatabaseAsset.h"

#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "UObject/UObjectHash.h"

#define LOCTEXT_NAMESPACE "KzDatabaseAssetValidator"

namespace KzDatabaseAssetValidator
{
	/** Component templates go by their variable; anything else by its class, such as an item's Database fragment. */
	FString OwnerLabel(const UObject* Owner)
	{
		const FString Name = Owner->GetName();
		return Name.EndsWith(TEXT("_GEN_VARIABLE")) ? Name.LeftChop(13) : Owner->GetClass()->GetDisplayNameText().ToString();
	}
}

bool UKzDatabaseAssetValidator::CanValidate_Implementation(const UObject* Asset) const
{
	return Asset && Asset->IsA<UKzDatabaseAsset>();
}

void UKzDatabaseAssetValidator::Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const
{
	const UKzDatabaseAsset* DatabaseAsset = Cast<UKzDatabaseAsset>(Asset);
	if (!DatabaseAsset)
	{
		return;
	}

	const FName Id = GetValidatorId();
	const FKzDatabase& Database = DatabaseAsset->Database;

	if (const UKzDatabaseAsset* Parent = DatabaseAsset->ParentDatabase)
	{
		if (Database.Type.IsValid() && Parent->Database.Type.IsValid() && Parent->Database.Type != Database.Type)
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("ParentType", "The parent table {0} holds another data type."), FText::FromString(Parent->GetName())), Id));
		}

		// Queries stop at a depth cap, but a loop means the hierarchy is not what it looks like.
		TSet<const UKzDatabaseAsset*> Visited = { DatabaseAsset };
		for (const UKzDatabaseAsset* Ancestor = Parent; Ancestor; Ancestor = Ancestor->ParentDatabase)
		{
			bool bAlreadyVisited = false;
			Visited.Add(Ancestor, &bAlreadyVisited);
			if (bAlreadyVisited)
			{
				OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("ParentLoop", "The parent chain loops back on itself at {0}."), FText::FromString(Ancestor->GetName())), Id));
				break;
			}
		}
	}

	KzDatabaseValidation::ValidateTable(Database, Id, OutIssues);
}

bool UKzDatabaseInlineValidator::CanValidate_Implementation(const UObject* Asset) const
{
	return Asset && (Asset->IsA<UBlueprint>() || Asset->IsA<UDataAsset>()) && !Asset->IsA<UKzDatabaseAsset>();
}

void UKzDatabaseInlineValidator::Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const
{
	// Where inline tables live: a Blueprint's defaults and component templates, or a data asset and its instanced subobjects, such as an item's fragments.
	TArray<const UObject*> Owners;
	if (const UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
	{
		if (Blueprint->GeneratedClass)
		{
			Owners.Add(Blueprint->GeneratedClass->GetDefaultObject());
		}
		if (Blueprint->SimpleConstructionScript)
		{
			for (const USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
			{
				if (Node && Node->ComponentTemplate)
				{
					Owners.Add(Node->ComponentTemplate);
				}
			}
		}
	}
	else
	{
		Owners.Add(Asset);
		TArray<UObject*> Subobjects;
		GetObjectsWithOuter(Asset, Subobjects, /*bIncludeNestedObjects=*/true);
		Owners.Append(Subobjects);
	}

	const FName Id = GetValidatorId();
	for (const UObject* Owner : Owners)
	{
		for (TPropertyValueIterator<FStructProperty> It(Owner->GetClass(), Owner); It; ++It)
		{
			if (It.Key()->Struct != FKzDatabase::StaticStruct())
			{
				continue;
			}
			It.SkipRecursiveProperty();

			TArray<FKzValidationIssue> TableIssues;
			KzDatabaseValidation::ValidateTable(*static_cast<const FKzDatabase*>(It.Value()), Id, TableIssues);

			const FText Where = FText::FromString(KzDatabaseAssetValidator::OwnerLabel(Owner));
			for (FKzValidationIssue& Issue : TableIssues)
			{
				// Row indices mean nothing to this asset's editor.
				Issue.ContextIndex = INDEX_NONE;
				Issue.Message = FText::Format(LOCTEXT("InlineIssue", "{0}: {1}"), Where, Issue.Message);
				OutIssues.Add(MoveTemp(Issue));
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE