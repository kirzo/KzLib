// Copyright 2026 kirzo

#include "Kismet/KzShuffleBagLibrary.h"

int32 UKzShuffleBagLibrary::DrawIndexFromShuffleBag(FKzShuffleBag& Bag)
{
	return Bag.Draw();
}

bool UKzShuffleBagLibrary::AddWeightToShuffleBag(FKzShuffleBag& Bag, int32 Weight)
{
	return Bag.AddWeight(Weight);
}

bool UKzShuffleBagLibrary::SetShuffleBagWeight(FKzShuffleBag& Bag, int32 Index, int32 Weight)
{
	if (!Bag.Entries.IsValidIndex(Index))
	{
		return false;
	}

	Bag.Entries[Index].Weight = Weight;
	return true;
}

void UKzShuffleBagLibrary::ResetShuffleBag(FKzShuffleBag& Bag)
{
	Bag.Reset();
}

void UKzShuffleBagLibrary::EmptyShuffleBag(FKzShuffleBag& Bag)
{
	Bag.Empty();
}

DEFINE_FUNCTION(UKzShuffleBagLibrary::execDrawFromShuffleBag)
{
	P_GET_STRUCT_REF(FKzShuffleBag, Bag);

	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	bool bDrawn = false;
	if (ValueProperty && ValuePtr)
	{
		if (!Bag.Type.IsValid())
		{
			FFrame::KismetExecutionMessage(TEXT("DrawFromShuffleBag: the bag holds weights alone; draw its index instead."), ELogVerbosity::Warning);
		}
		else if (const int32 Index = Bag.Draw(); Index != INDEX_NONE)
		{
			bDrawn = Bag.Entries[Index].Value.ToProperty(ValueProperty, ValuePtr);
			if (!bDrawn)
			{
				FFrame::KismetExecutionMessage(*FString::Printf(TEXT("DrawFromShuffleBag: the pin is '%s', not of the bag's type."), *ValueProperty->GetCPPType()), ELogVerbosity::Warning);
			}
		}
	}

	*(bool*)RESULT_PARAM = bDrawn;
}

DEFINE_FUNCTION(UKzShuffleBagLibrary::execAddToShuffleBag)
{
	P_GET_STRUCT_REF(FKzShuffleBag, Bag);

	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	const void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FIntProperty, Weight);

	P_FINISH;

	bool bAdded = false;
	if (ValueProperty && ValuePtr)
	{
		bAdded = (!Bag.Type.IsValid() || Bag.Type.MatchesProperty(ValueProperty)) && Bag.Add(FKzVariant::FromProperty(ValueProperty, ValuePtr), Weight);
		if (!bAdded)
		{
			FFrame::KismetExecutionMessage(*FString::Printf(TEXT("AddToShuffleBag: a '%s' doesn't fit the bag's type, or the bag holds weights alone."), *ValueProperty->GetCPPType()), ELogVerbosity::Warning);
		}
	}

	*(bool*)RESULT_PARAM = bAdded;
}