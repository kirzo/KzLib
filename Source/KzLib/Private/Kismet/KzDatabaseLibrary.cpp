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

static bool ArePropertiesCompatible(const FProperty* InputProp, const FProperty* TargetProp)
{
	// 1. Basic class check
	if (InputProp->GetClass() != TargetProp->GetClass())
	{
		// Special case: Byte vs Enum
		if ((InputProp->IsA<FByteProperty>() && TargetProp->IsA<FEnumProperty>()) ||
			(InputProp->IsA<FEnumProperty>() && TargetProp->IsA<FByteProperty>()))
		{
			return true;
		}
		return false;
	}

	// 2. Struct Check
	if (const FStructProperty* InputStruct = CastField<FStructProperty>(InputProp))
	{
		const FStructProperty* TargetStruct = CastField<const FStructProperty>(TargetProp);
		return InputStruct->Struct == TargetStruct->Struct;
	}

	// 3. Object Check (Allow Child -> Parent)
	if (const FObjectPropertyBase* InputObj = CastField<FObjectPropertyBase>(InputProp))
	{
		const FObjectPropertyBase* TargetObj = CastField<const FObjectPropertyBase>(TargetProp);
		return InputObj->PropertyClass->IsChildOf(TargetObj->PropertyClass);
	}

	// 4. Array Check
	if (const FArrayProperty* InputArray = CastField<FArrayProperty>(InputProp))
	{
		const FArrayProperty* TargetArray = CastField<const FArrayProperty>(TargetProp);
		return ArePropertiesCompatible(InputArray->Inner, TargetArray->Inner);
	}

	// 5. Enum Check
	if (const FEnumProperty* InputEnum = CastField<FEnumProperty>(InputProp))
	{
		const FEnumProperty* TargetEnum = CastField<const FEnumProperty>(TargetProp);
		return InputEnum->GetEnum() == TargetEnum->GetEnum();
	}

	return true;
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

	if (StackProp && StackPtr)
	{
		// Validate Schema Compatibility BEFORE modification.
		// We create a temporary lightweight bag definition matching the Database Type.
		// This avoids adding "trash" items to the array if the type is wrong.

		FInstancedPropertyBag SchemaValidationBag;
		static const FName PropName("Data");

		if (Database.Type.IsValid())
		{
			SchemaValidationBag.AddProperty(PropName, Database.Type.ValueType, Database.Type.ValueTypeObject.Get());
		}

		const UScriptStruct* SchemaStruct = SchemaValidationBag.GetPropertyBagStruct();
		const FProperty* SchemaProp = SchemaStruct ? SchemaStruct->FindPropertyByName(PropName) : nullptr;

		if (SchemaProp && ArePropertiesCompatible(StackProp, SchemaProp))
		{
			FKzDatabaseItem* Item = Database.FindItem(ID);
			if (!Item)
			{
				Item = &Database.Items.AddDefaulted_GetRef();
				Item->ID = ID;
			}

			Item->Tags = Tags;
			Item->SyncType(Database.Type); // Ensure internal memory matches Schema

			FStructView MutableStruct = Item->Data.GetMutableValue();
			uint8* StructMemory = MutableStruct.GetMemory();

			if (StructMemory)
			{
				const FProperty* RealBagProp = Item->Data.GetPropertyBagStruct()->FindPropertyByName(PropName);

				if (RealBagProp)
				{
					uint8* DestPtr = RealBagProp->ContainerPtrToValuePtr<uint8>(StructMemory);
					RealBagProp->CopyCompleteValue(DestPtr, StackPtr);
					bSuccess = true;
				}
			}
		}
		else
		{
			FString ExpectedType = SchemaProp ? SchemaProp->GetCPPType() : TEXT("Invalid Schema");
			FFrame::KismetExecutionMessage(
				*FString::Printf(TEXT("AddDatabaseItem: Type mismatch for ID '%s'. DB expects '%s', input is '%s'."),
					*ID.ToString(), *ExpectedType, *StackProp->GetCPPType()),
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

	if (Item && Item->IsValid() && StackProp && StackPtr)
	{
		static const FName PropName("Data");
		const UScriptStruct* BagStruct = Item->Data.GetPropertyBagStruct();
		const FProperty* BagProp = BagStruct ? BagStruct->FindPropertyByName(PropName) : nullptr;

		if (BagProp)
		{
			if (ArePropertiesCompatible(StackProp, BagProp))
			{
				FStructView MutableStruct = Item->Data.GetMutableValue();
				uint8* StructMemory = MutableStruct.GetMemory();

				if (StructMemory)
				{
					uint8* DestPtr = BagProp->ContainerPtrToValuePtr<uint8>(StructMemory);
					BagProp->CopyCompleteValue(DestPtr, StackPtr);
					bSuccess = true;
				}
			}
			else
			{
				FFrame::KismetExecutionMessage(
					*FString::Printf(TEXT("SetDatabaseItemValue: Type mismatch for ID '%s'. DB expects '%s', input is '%s'."),
						*ID.ToString(), *BagProp->GetCPPType(), *StackProp->GetCPPType()),
					ELogVerbosity::Warning
				);
			}
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

	if (Item && Item->IsValid() && StackProp && StackPtr)
	{
		static const FName PropName("Data");
		const UScriptStruct* BagStruct = Item->Data.GetPropertyBagStruct();
		const FProperty* BagProp = BagStruct ? BagStruct->FindPropertyByName(PropName) : nullptr;

		// Verify compatibility (Source -> Dest)
		if (BagProp && ArePropertiesCompatible(BagProp, StackProp))
		{
			// Get Const Memory
			FConstStructView ConstStruct = Item->Data.GetValue();
			const uint8* StructMemory = ConstStruct.GetMemory();

			if (StructMemory)
			{
				const uint8* SrcPtr = BagProp->ContainerPtrToValuePtr<uint8>(StructMemory);

				// Copy from Bag to Stack
				StackProp->CopyCompleteValue(StackPtr, SrcPtr);
				bSuccess = true;
			}
		}
	}

	*(bool*)RESULT_PARAM = bSuccess;
}

DEFINE_FUNCTION(UKzDatabaseLibrary::execFindBestMatch)
{
	P_GET_STRUCT_REF(FKzDatabase, Database);
	P_GET_STRUCT_REF(FKzDatabaseQuery, Query);
	P_GET_PROPERTY_REF(FNameProperty, OutID);

	// Wildcard Value output
	FProperty* ValueProp = nullptr;
	void* ValuePtr = nullptr;
	GetWildcardProperty(Stack, ValueProp, ValuePtr);

	P_FINISH;

	bool bFound = false;

	if (const FKzDatabaseItem* BestItem = Database.FindBestMatch(Query))
	{
		OutID = BestItem->ID;

		// Write Value
		if (ValueProp && ValuePtr && BestItem->IsValid())
		{
			static const FName PropName("Data");
			const UScriptStruct* BagStruct = BestItem->Data.GetPropertyBagStruct();
			const FProperty* BagProp = BagStruct ? BagStruct->FindPropertyByName(PropName) : nullptr;

			if (BagProp && ArePropertiesCompatible(BagProp, ValueProp))
			{
				FConstStructView ConstStruct = BestItem->Data.GetValue();
				const uint8* SrcPtr = BagProp->ContainerPtrToValuePtr<uint8>(ConstStruct.GetMemory());
				if (SrcPtr)
				{
					ValueProp->CopyCompleteValue(ValuePtr, SrcPtr);
					bFound = true;
				}
			}
		}
		else
		{
			// Found item but didn't write value (maybe pin not connected or types mismatch), still success finding item
			bFound = true;
		}
	}

	*(bool*)RESULT_PARAM = bFound;
}

DEFINE_FUNCTION(UKzDatabaseLibrary::execEvaluateDatabaseAsset)
{
	P_GET_OBJECT(UKzDatabaseAsset, Asset);
	P_GET_STRUCT_REF(FKzDatabaseQuery, Query);

	FProperty* OutValueProp = nullptr;
	void* OutValuePtr = nullptr;
	GetWildcardProperty(Stack, OutValueProp, OutValuePtr);

	P_FINISH;

	bool bSuccess = false;

	if (Asset && OutValueProp && OutValuePtr)
	{
		if (const FKzDatabaseItem* BestItem = Asset->ResolveMatch(Query))
		{
			if (BestItem->IsValid())
			{
				static const FName PropName("Data");
				const UScriptStruct* BagStruct = BestItem->Data.GetPropertyBagStruct();
				const FProperty* BagProp = BagStruct ? BagStruct->FindPropertyByName(PropName) : nullptr;

				// Type validation (Asset Schema vs Node Pin)
				if (BagProp && ArePropertiesCompatible(BagProp, OutValueProp))
				{
					FConstStructView ConstStruct = BestItem->Data.GetValue();
					const uint8* SrcPtr = BagProp->ContainerPtrToValuePtr<uint8>(ConstStruct.GetMemory());

					if (SrcPtr)
					{
						// Copy memory directly to the Blueprint pin
						OutValueProp->CopyCompleteValue(OutValuePtr, SrcPtr);
						bSuccess = true;
					}
				}
				else
				{
					FFrame::KismetExecutionMessage(
						*FString::Printf(TEXT("EvaluateDatabaseAsset: Type mismatch on Asset '%s'. Asset provides '%s', but node pin is '%s'."),
							*Asset->GetName(), BagProp ? *BagProp->GetCPPType() : TEXT("Unknown"), *OutValueProp->GetCPPType()),
						ELogVerbosity::Warning
					);
				}
			}
		}
	}

	*(bool*)RESULT_PARAM = bSuccess;
}

DEFINE_FUNCTION(UKzDatabaseLibrary::execResolveDatabaseQuery)
{
	P_GET_OBJECT(UKzDatabaseComponent, Component);
	P_GET_STRUCT_REF(FKzDatabaseQuery, Query);

	FProperty* OutValueProp = nullptr;
	void* OutValuePtr = nullptr;
	GetWildcardProperty(Stack, OutValueProp, OutValuePtr);

	P_FINISH;

	bool bSuccess = false;

	if (Component && OutValueProp && OutValuePtr)
	{
		FPropertyBagPropertyDesc Desc(NAME_None, OutValueProp);
		FKzParamDef SearchType(Desc.Name, Desc.ContainerTypes.GetFirstContainerType(), Desc.ValueType, Desc.ValueTypeObject.Get());

		if (UKzDatabaseAsset* Asset = Component->GetDatabaseAsset(SearchType))
		{
			if (const FKzDatabaseItem* BestItem = Asset->ResolveMatch(Query))
			{
				if (BestItem->IsValid())
				{
					static const FName PropName("Data");
					const UScriptStruct* BagStruct = BestItem->Data.GetPropertyBagStruct();
					const FProperty* BagProp = BagStruct ? BagStruct->FindPropertyByName(PropName) : nullptr;

					if (BagProp && ArePropertiesCompatible(BagProp, OutValueProp))
					{
						FConstStructView ConstStruct = BestItem->Data.GetValue();
						const uint8* SrcPtr = BagProp->ContainerPtrToValuePtr<uint8>(ConstStruct.GetMemory());

						if (SrcPtr)
						{
							OutValueProp->CopyCompleteValue(OutValuePtr, SrcPtr);
							bSuccess = true;
						}
					}
				}
			}
		}
	}

	*(bool*)RESULT_PARAM = bSuccess;
}