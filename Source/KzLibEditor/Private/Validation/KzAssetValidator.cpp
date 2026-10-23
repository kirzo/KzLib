// Copyright 2026 kirzo

#include "Validation/KzAssetValidator.h"

FName UKzAssetValidator::GetValidatorId_Implementation() const
{
	FString Name = GetClass()->GetName();
	Name.RemoveFromEnd(TEXT("_C"));

	int32 Underscore = INDEX_NONE;
	if (Name.FindLastChar(TEXT('_'), Underscore))
	{
		Name.RightChopInline(Underscore + 1);
	}
	return FName(*FName::NameToDisplayString(Name, false));
}