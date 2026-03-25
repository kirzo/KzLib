// Copyright 2026 kirzo

#include "Core/KzDatabaseAsset.h"

#if WITH_EDITOR
#include "Misc/MessageDialog.h"
#endif

const FKzDatabaseItem* UKzDatabaseAsset::ResolveMatch(const FKzDatabaseQuery& Query) const
{
	int32 BestScore = -1;
	return ResolveBestMatchRecursive(Query, BestScore);
}

const FKzDatabaseItem* UKzDatabaseAsset::ResolveBestMatchRecursive(const FKzDatabaseQuery& Query, int32& OutBestScore) const
{
	// 1. Find the best match locally in this Asset
	const FKzDatabaseItem* LocalBestItem = Database.FindBestMatch(Query);
	int32 LocalScore = -1;

	if (LocalBestItem)
	{
		LocalScore = Query.CalculateScore(LocalBestItem->Tags);
	}

	// 2. Delegate to the parent layout, if any
	const FKzDatabaseItem* ParentBestItem = nullptr;
	int32 ParentScore = -1;

	if (ParentDatabase && ParentDatabase != this)
	{
		ParentBestItem = ParentDatabase->ResolveBestMatchRecursive(Query, ParentScore);
	}

	// 3. Compare scores and return the winner.
	// Note: Local ties win over Parent ties. A child override with the same score takes precedence.
	if (ParentBestItem && ParentScore > LocalScore)
	{
		OutBestScore = ParentScore;
		return ParentBestItem;
	}

	OutBestScore = LocalScore;
	return LocalBestItem;
}

#if WITH_EDITOR
void UKzDatabaseAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// Validate changes to the Parent Database
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UKzDatabaseAsset, ParentDatabase))
	{
		if (ParentDatabase)
		{
			// 1. Prevent Self-Reference
			if (ParentDatabase == this)
			{
				ParentDatabase = nullptr;
				FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("A Database Asset cannot be its own parent."));
				return;
			}

			// 2. Prevent Circular Dependencies (A -> B -> C -> A)
			if (HasCircularDependency(ParentDatabase))
			{
				ParentDatabase = nullptr;
				FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Circular dependency detected! This asset is already a parent somewhere in that hierarchy."));
				return;
			}

			// 3. Prevent Type Mismatch
			// We only check if both assets have been properly initialized with a type
			if (Database.Type.IsValid() && ParentDatabase->GetDataType().IsValid())
			{
				if (Database.Type != ParentDatabase->GetDataType())
				{
					ParentDatabase = nullptr;
					FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Type Mismatch! The parent database must contain the exact same data type as this database."));
					return;
				}
			}
		}
	}
}

bool UKzDatabaseAsset::HasCircularDependency(const UKzDatabaseAsset* PotentialParent) const
{
	const UKzDatabaseAsset* CurrentNode = PotentialParent;

	// Walk up the chain to see if we eventually hit 'this'
	while (CurrentNode)
	{
		if (CurrentNode == this)
		{
			return true;
		}
		CurrentNode = CurrentNode->ParentDatabase;
	}

	return false;
}
#endif