// Copyright 2026 kirzo

#include "Kismet/KzDatabaseLibrary.h"
#include "StructUtils/StructView.h"
#include "Core/KzDatabaseAsset.h"
#include "Components/KzDatabaseComponent.h"

// --- Standard Functions ---

bool UKzDatabaseLibrary::RemoveDatabaseItem(FKzDatabase& Database, FName ID)
{
	return Database.RemoveItem(ID);
}

void UKzDatabaseLibrary::EmptyDatabase(FKzDatabase& Database)
{
	Database.Empty();
}

bool UKzDatabaseLibrary::IsDatabaseEmpty(const FKzDatabase& Database)
{
	return Database.IsEmpty();
}

// --- Custom Thunks Helpers ---

static void GetWildcardProperty(FFrame& Stack, FProperty*& OutProp, void*& OutPtr)
{
	Stack.StepCompiledIn<FProperty>(nullptr);
	OutProp = Stack.MostRecentProperty;
	OutPtr = Stack.MostRecentPropertyAddress;
}

// --- Thunk Implementations ---

DEFINE_FUNCTION(UKzDatabaseLibrary::execAddDatabaseItem)
{
	P_GET_STRUCT_REF(FKzDatabase, Database);
	P_GET_PROPERTY(FNameProperty, ID);
	P_GET_STRUCT(FGameplayTagContainer, Tags);

	FProperty* StackProp = nullptr;
	void* StackPtr = nullptr;
	GetWildcardProperty(Stack, StackProp, StackPtr);

	P_FINISH;

	bool bSuccess = false;

	if (StackProp && StackPtr && Database.Type.IsValid())
	{
		// Validate compatibility against the database's declared type before touching the items array.
		if (Database.Type.MatchesProperty(StackProp))
		{
			FKzDatabaseItem* Item = Database.FindItem(ID);
			if (!Item)
			{
				Item = &Database.Items.AddDefaulted_GetRef();
				Item->ID = ID;
			}

			Item->Tags = Tags;
			Item->Value = FKzVariant::FromProperty(StackProp, StackPtr);
			bSuccess = Item->Value.IsValid();
		}
		else
		{
			FFrame::KismetExecutionMessage(
				*FString::Printf(TEXT("AddDatabaseItem: Type mismatch for ID '%s'. Input is '%s'."),
					*ID.ToString(), *StackProp->GetCPPType()),
				ELogVerbosity::Warning
			);
		}
	}

	*(bool*)RESULT_PARAM = bSuccess;
}

DEFINE_FUNCTION(UKzDatabaseLibrary::execSetDatabaseItemValue)
{
	P_GET_STRUCT_REF(FKzDatabase, Database);
	P_GET_PROPERTY(FNameProperty, ID);

	FProperty* StackProp = nullptr;
	void* StackPtr = nullptr;
	GetWildcardProperty(Stack, StackProp, StackPtr);

	P_FINISH;

	bool bSuccess = false;
	FKzDatabaseItem* Item = Database.FindItem(ID);

	if (Item && StackProp && StackPtr && Database.Type.IsValid())
	{
		if (Database.Type.MatchesProperty(StackProp))
		{
			Item->Value = FKzVariant::FromProperty(StackProp, StackPtr);
			bSuccess = Item->Value.IsValid();
		}
		else
		{
			FFrame::KismetExecutionMessage(
				*FString::Printf(TEXT("SetDatabaseItemValue: Type mismatch for ID '%s'. Input is '%s'."),
					*ID.ToString(), *StackProp->GetCPPType()),
				ELogVerbosity::Warning
			);
		}
	}

	*(bool*)RESULT_PARAM = bSuccess;
}

DEFINE_FUNCTION(UKzDatabaseLibrary::execGetDatabaseItemValue)
{
	P_GET_STRUCT_REF(FKzDatabase, Database);
	P_GET_PROPERTY(FNameProperty, ID);

	FProperty* StackProp = nullptr;
	void* StackPtr = nullptr;
	GetWildcardProperty(Stack, StackProp, StackPtr);

	P_FINISH;

	bool bSuccess = false;
	const FKzDatabaseItem* Item = Database.FindItem(ID);

	if (Item && Item->Value.IsValid() && StackProp && StackPtr)
	{
		bSuccess = Item->Value.ToProperty(StackProp, StackPtr);
	}

	*(bool*)RESULT_PARAM = bSuccess;
}

DEFINE_FUNCTION(UKzDatabaseLibrary::execFindBestMatch)
{
	P_GET_STRUCT_REF(FKzDatabase, Database);
	P_GET_STRUCT_REF(FKzDatabaseQuery, Query);
	P_GET_PROPERTY_REF(FNameProperty, OutID);

	FProperty* ValueProp = nullptr;
	void* ValuePtr = nullptr;
	GetWildcardProperty(Stack, ValueProp, ValuePtr);

	P_FINISH;

	bool bFound = false;

	if (const FKzDatabaseItem* BestItem = Database.FindBestMatch(Query))
	{
		OutID = BestItem->ID;
		bFound = true;

		// Write the value out if the pin is connected and compatible. A type mismatch doesn't fail the search,
		// it just leaves the out pin untouched.
		if (ValueProp && ValuePtr && BestItem->Value.IsValid())
		{
			BestItem->Value.ToProperty(ValueProp, ValuePtr);
		}
	}

	*(bool*)RESULT_PARAM = bFound;
}

DEFINE_FUNCTION(UKzDatabaseLibrary::execEvaluateDatabaseAsset)
{
	P_GET_OBJECT(UKzDatabaseAsset, Asset);
	P_GET_STRUCT_REF(FKzDatabaseQuery, Query);
	P_GET_ENUM_REF(EKzSearchResult, Result);

	FProperty* OutValueProp = nullptr;
	void* OutValuePtr = nullptr;
	GetWildcardProperty(Stack, OutValueProp, OutValuePtr);

	P_FINISH;

	Result = EKzSearchResult::NotFound;

	if (Asset && OutValueProp && OutValuePtr)
	{
		if (const FKzDatabaseItem* BestItem = Asset->ResolveMatch(Query))
		{
			if (BestItem->Value.IsValid())
			{
				if (BestItem->Value.MatchesProperty(OutValueProp))
				{
					if (BestItem->Value.ToProperty(OutValueProp, OutValuePtr))
					{
						Result = EKzSearchResult::Found;
					}
				}
				else
				{
					FFrame::KismetExecutionMessage(
						*FString::Printf(TEXT("EvaluateDatabaseAsset: Type mismatch on Asset '%s'. Node pin is '%s'."),
							*Asset->GetName(), *OutValueProp->GetCPPType()),
						ELogVerbosity::Warning
					);
				}
			}
		}
	}
}

DEFINE_FUNCTION(UKzDatabaseLibrary::execResolveDatabaseQuery)
{
	P_GET_OBJECT(UKzDatabaseComponent, Component);
	P_GET_STRUCT_REF(FKzDatabaseQuery, Query);
	P_GET_ENUM_REF(EKzSearchResult, Result);

	FProperty* OutValueProp = nullptr;
	void* OutValuePtr = nullptr;
	GetWildcardProperty(Stack, OutValueProp, OutValuePtr);

	P_FINISH;

	Result = EKzSearchResult::NotFound;

	if (Component && OutValueProp && OutValuePtr)
	{
		// Build the search type from the wildcard pin's FProperty.
		FPropertyBagPropertyDesc Desc(NAME_None, OutValueProp);
		const FKzTypeDef SearchType(Desc.ContainerTypes.GetFirstContainerType(), Desc.ValueType, Desc.ValueTypeObject.Get());

		if (const FKzDatabaseItem* BestItem = Component->ResolveItem(SearchType, Query))
		{
			if (BestItem->Value.IsValid() && BestItem->Value.MatchesProperty(OutValueProp))
			{
				if (BestItem->Value.ToProperty(OutValueProp, OutValuePtr))
				{
					Result = EKzSearchResult::Found;
				}
			}
		}
	}
}