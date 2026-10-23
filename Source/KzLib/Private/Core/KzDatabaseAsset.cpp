// Copyright 2026 kirzo

#include "Core/KzDatabaseAsset.h"

#if WITH_EDITOR
#include "Misc/MessageDialog.h"
#endif

namespace KzDatabaseAsset
{
	/** Deeper than any real hierarchy; stops a parent cycle in corrupt data. */
	constexpr int32 MaxDepth = 32;
}

const FKzDatabaseItem* UKzDatabaseAsset::ResolveMatch(const FKzDatabaseQuery& Query, int32* OutScore) const
{
	const FKzDatabaseItem* BestItem = nullptr;
	int32 BestScore = -1;

	// From child to root, so a parent's item only wins with a strictly better score.
	int32 Depth = 0;
	for (const UKzDatabaseAsset* Asset = this; Asset && Depth < KzDatabaseAsset::MaxDepth; Asset = Asset->ParentDatabase, ++Depth)
	{
		int32 Score = -1;
		const FKzDatabaseItem* Item = Asset->Database.FindBestMatch(Query, &Score);
		if (Item && Score > BestScore)
		{
			BestItem = Item;
			BestScore = Score;
		}
	}

	if (OutScore)
	{
		*OutScore = BestScore;
	}
	return BestItem;
}

void UKzDatabaseAsset::GetSoftValuePaths(TArray<FSoftObjectPath>& OutPaths) const
{
	int32 Depth = 0;
	for (const UKzDatabaseAsset* Asset = this; Asset && Depth < KzDatabaseAsset::MaxDepth; Asset = Asset->ParentDatabase, ++Depth)
	{
		Asset->Database.GetSoftValuePaths(OutPaths);
	}
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
	// Walk up the chain to see if we eventually hit 'this'; the cap also catches a cycle that doesn't.
	int32 Depth = 0;
	for (const UKzDatabaseAsset* CurrentNode = PotentialParent; CurrentNode; CurrentNode = CurrentNode->ParentDatabase)
	{
		if (CurrentNode == this || ++Depth >= KzDatabaseAsset::MaxDepth)
		{
			return true;
		}
	}

	return false;
}
#endif