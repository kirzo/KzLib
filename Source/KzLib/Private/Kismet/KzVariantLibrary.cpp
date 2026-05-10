// Copyright 2026 kirzo

#include "Kismet/KzVariantLibrary.h"

FKzVariant UKzVariantLibrary::MakeKzVariant(const int32& Value)
{
	checkNoEntry();
	return FKzVariant();
}

FKzVariant& UKzVariantLibrary::SetKzVariant(FKzVariant& Variant, const int32& Value)
{
	checkNoEntry();
	return Variant;
}

bool UKzVariantLibrary::GetKzVariant(const FKzVariant& Variant, int32& OutValue)
{
	checkNoEntry();
	return false;
}

void UKzVariantLibrary::BreakKzVariant(EKzValidity& ExecResult, const FKzVariant& Variant, int32& Value)
{
	checkNoEntry();
}

// =====================================================================================
// Custom thunks
// =====================================================================================

DEFINE_FUNCTION(UKzVariantLibrary::execMakeKzVariant)
{
	// Read the wildcard parameter. Stack.MostRecentProperty/Address are populated by the previous step's evaluation.
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	const void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	*static_cast<FKzVariant*>(RESULT_PARAM) = FKzVariant::FromProperty(ValueProperty, ValuePtr);
}

DEFINE_FUNCTION(UKzVariantLibrary::execSetKzVariant)
{
	P_GET_STRUCT_REF(FKzVariant, Variant);

	// Second param: wildcard in.
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	const void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	if (ValueProperty && ValuePtr)
	{
		Variant = FKzVariant::FromProperty(ValueProperty, ValuePtr);
	}

	*(FKzVariant*)RESULT_PARAM = Variant;
}

DEFINE_FUNCTION(UKzVariantLibrary::execGetKzVariant)
{
	P_GET_STRUCT_REF(FKzVariant, Variant);

	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* OutProperty = Stack.MostRecentProperty;
	void* OutPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	bool bSuccess = false;
	if (OutProperty && OutPtr)
	{
		bSuccess = Variant.ToProperty(OutProperty, OutPtr);
	}

	*static_cast<bool*>(RESULT_PARAM) = bSuccess;
}

DEFINE_FUNCTION(UKzVariantLibrary::execBreakKzVariant)
{
	P_GET_ENUM_REF(EKzValidity, ExecResult);
	P_GET_STRUCT_REF(FKzVariant, Variant);

	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);

	void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	ExecResult = EKzValidity::NotValid;

	if (Variant.MatchesProperty(Stack.MostRecentProperty))
	{
		Stack.MostRecentProperty->CopyCompleteValue(ValuePtr, Variant.GetData());
		ExecResult = EKzValidity::Valid;
	}
}