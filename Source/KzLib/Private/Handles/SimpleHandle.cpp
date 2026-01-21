// Copyright 2026 kirzo

#include "Handles/SimpleHandle.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

static UScriptStruct* StaticGetBaseStructureInternal(FName Name)
{
	static UPackage* Pkg = FindObjectChecked<UPackage>(nullptr, TEXT("/Script/KzLib"));

	UScriptStruct* Result = (UScriptStruct*)StaticFindObjectFastInternal(UScriptStruct::StaticClass(), Pkg, Name, false, RF_NoFlags, EInternalObjectFlags::None);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!Result)
	{
		UE_LOG(LogClass, Fatal, TEXT("Failed to find native struct '%s.%s'"), *Pkg->GetName(), *Name.ToString());
	}
#endif
	return Result;
}

bool FSimpleHandle::Identical(const FSimpleHandle* Other, uint32 PortFlags) const
{
	return Other && (*this == *Other);
}

UScriptStruct* TBaseStructure<FSimpleHandle>::Get()
{
	static UScriptStruct* ScriptStruct = StaticGetBaseStructureInternal(TEXT("SimpleHandle"));
	return ScriptStruct;
}