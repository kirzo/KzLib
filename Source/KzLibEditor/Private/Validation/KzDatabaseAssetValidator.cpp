// Copyright 2026 kirzo

#include "Validation/KzDatabaseAssetValidator.h"
#include "Core/KzDatabaseAsset.h"

#define LOCTEXT_NAMESPACE "KzDatabaseAssetValidator"

namespace KzDatabaseAssetValidator
{
	FText RowLabel(int32 Index, const FKzDatabaseItem& Item)
	{
		return Item.ID.IsNone()
			? FText::Format(LOCTEXT("Row", "Row {0}"), FText::AsNumber(Index + 1))
			: FText::Format(LOCTEXT("RowWithId", "Row {0} ({1})"), FText::AsNumber(Index + 1), FText::FromName(Item.ID));
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

	if (!Database.Type.IsValid())
	{
		OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("NoType", "The table has no data type."), Id));
	}

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

	TMap<FName, int32> FirstWithId;
	for (int32 Index = 0; Index < Database.Items.Num(); ++Index)
	{
		const FKzDatabaseItem& Item = Database.Items[Index];
		const FText Row = KzDatabaseAssetValidator::RowLabel(Index, Item);

		if (!Item.ID.IsNone())
		{
			if (const int32* Earlier = FirstWithId.Find(Item.ID))
			{
				OutIssues.Add(FKzValidationIssue::WithContextIndex(EKzValidationSeverity::Error,
					FText::Format(LOCTEXT("DuplicateId", "{0} repeats the ID of row {1}; a lookup by ID finds row {1}."), Row, FText::AsNumber(*Earlier + 1)), Id, Index));
			}
			else
			{
				FirstWithId.Add(Item.ID, Index);
			}
		}

		if (Database.Type.IsValid())
		{
			const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Item.Value.GetValueProperty());
			if (!Item.Value.MatchesType(Database.Type))
			{
				OutIssues.Add(FKzValidationIssue::WithContextIndex(EKzValidationSeverity::Error,
					FText::Format(LOCTEXT("WrongType", "{0} holds a value of another type than the table's."), Row), Id, Index));
			}
			else if (ObjectProperty && ObjectProperty->Identical(Item.Value.GetData(), nullptr, PPF_None))
			{
				OutIssues.Add(FKzValidationIssue::WithContextIndex(EKzValidationSeverity::Warning,
					FText::Format(LOCTEXT("NoValue", "{0} has no value set."), Row), Id, Index));
			}
		}

		if (Item.Tags.IsEmpty())
		{
			OutIssues.Add(FKzValidationIssue::WithContextIndex(EKzValidationSeverity::Info,
				FText::Format(LOCTEXT("NoTags", "{0} has no tags: only queries that require nothing can pick it."), Row), Id, Index));
		}

		// A tie goes to the earlier row, so an exact twin never wins.
		for (int32 EarlierIndex = 0; EarlierIndex < Index; ++EarlierIndex)
		{
			if (Database.Items[EarlierIndex].Tags == Item.Tags)
			{
				OutIssues.Add(FKzValidationIssue::WithContextIndex(EKzValidationSeverity::Warning,
					FText::Format(LOCTEXT("Shadowed", "{0} has the same tags as row {1}, which always wins over it."), Row, FText::AsNumber(EarlierIndex + 1)), Id, Index));
				break;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE