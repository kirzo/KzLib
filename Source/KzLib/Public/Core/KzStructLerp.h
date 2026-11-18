// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

class UScriptStruct;

namespace Kz
{
	/**
	 * Blends B into Out, both instances of Struct: Out = Lerp(Out, B, Alpha).
	 * Floats, doubles, ints and the structs made of them (vectors, linear colors) interpolate; FColor does in linear space,
	 * FRotator and FQuat through quaternions. Anything else (bools, enums, names, objects, containers) takes B's value from Alpha 0.5.
	 * A property with a bool bOverride_<Name> next to it only blends where B sets that flag, and then sets it on Out.
	 * Members that aren't UPROPERTYs are left alone. Each struct's layout is worked out once, on the game thread, and cached.
	 */
	KZLIB_API void LerpStruct(const UScriptStruct* Struct, void* Out, const void* B, float Alpha);
}