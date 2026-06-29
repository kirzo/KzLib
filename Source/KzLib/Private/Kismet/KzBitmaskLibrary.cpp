// Copyright 2026 kirzo

#include "Kismet/KzBitmaskLibrary.h"

// === Integer flags ===

int32 UKzBitmaskLibrary::MakeFlag(int32 Index)
{
	// Shift on unsigned to avoid signed-shift UB at bit 31, then reinterpret as int32.
	return (Index >= 0 && Index < 32) ? static_cast<int32>(1u << Index) : 0;
}

bool UKzBitmaskLibrary::ContainsFlag(int32 Mask, int32 Flag)
{
	return Flag != 0 && (Mask & Flag) == Flag;
}

bool UKzBitmaskLibrary::ContainsAnyFlags(int32 Mask, int32 Flags)
{
	return (Mask & Flags) != 0;
}

bool UKzBitmaskLibrary::ContainsAllFlags(int32 Mask, int32 Flags)
{
	return Flags != 0 && (Mask & Flags) == Flags;
}

bool UKzBitmaskLibrary::ContainsFlagAtIndex(int32 Mask, int32 Index)
{
	return (Index >= 0 && Index < 32) && (Mask & static_cast<int32>(1u << Index)) != 0;
}

bool UKzBitmaskLibrary::MarkFlags(int32& Mask, int32 Flags)
{
	const int32 Old = Mask;
	Mask |= Flags;
	return Mask != Old; // an OR only changes the mask when some bit went 0->1
}

bool UKzBitmaskLibrary::MarkFlagAtIndex(int32& Mask, int32 Index)
{
	if (Index < 0 || Index >= 32) { return false; }
	const int32 Old = Mask;
	Mask |= static_cast<int32>(1u << Index);
	return Mask != Old;
}

bool UKzBitmaskLibrary::ClearFlags(int32& Mask, int32 Flags)
{
	const int32 Old = Mask;
	Mask &= ~Flags;
	return Mask != Old; // changed only when some bit went 1->0
}

bool UKzBitmaskLibrary::ClearFlagAtIndex(int32& Mask, int32 Index)
{
	if (Index < 0 || Index >= 32) { return false; }
	const int32 Old = Mask;
	Mask &= ~static_cast<int32>(1u << Index);
	return Mask != Old;
}

int32 UKzBitmaskLibrary::SetFlagAtIndex(int32 Mask, int32 Index, bool bSet)
{
	if (Index < 0 || Index >= 32) { return Mask; }
	const int32 Bit = static_cast<int32>(1u << Index);
	return bSet ? (Mask | Bit) : (Mask & ~Bit);
}

int32 UKzBitmaskLibrary::RemoveFlags(int32 Mask, int32 Flags)
{
	return Mask & ~Flags;
}

int32 UKzBitmaskLibrary::CountSetFlags(int32 Mask)
{
	return FMath::CountBits(static_cast<uint64>(static_cast<uint32>(Mask)));
}

// === Integer shifts ===

int32 UKzBitmaskLibrary::ShiftLeft(int32 Value, int32 Count)
{
	if (Count <= 0) { return Value; }                  // 0 or negative -> no shift
	if (Count >= 32) { return 0; }                     // shifted entirely out
	// Shift on unsigned to avoid signed-shift UB, then reinterpret.
	return static_cast<int32>(static_cast<uint32>(Value) << Count);
}

int32 UKzBitmaskLibrary::ShiftRight(int32 Value, int32 Count)
{
	if (Count <= 0) { return Value; }
	if (Count >= 32) { return 0; }
	// Logical (zero-fill) shift: operate on unsigned so the sign bit isn't propagated.
	return static_cast<int32>(static_cast<uint32>(Value) >> Count);
}

// === Byte (uint8) flags ===

uint8 UKzBitmaskLibrary::MakeFlagByte(int32 Index)
{
	return (Index >= 0 && Index < 8) ? static_cast<uint8>(1u << Index) : 0;
}

bool UKzBitmaskLibrary::ContainsFlagByte(uint8 Mask, uint8 Flag)
{
	return Flag != 0 && (Mask & Flag) == Flag;
}

bool UKzBitmaskLibrary::ContainsAnyFlagsByte(uint8 Mask, uint8 Flags)
{
	return (Mask & Flags) != 0;
}

bool UKzBitmaskLibrary::ContainsAllFlagsByte(uint8 Mask, uint8 Flags)
{
	return Flags != 0 && (Mask & Flags) == Flags;
}

bool UKzBitmaskLibrary::ContainsFlagAtIndexByte(uint8 Mask, int32 Index)
{
	return (Index >= 0 && Index < 8) && (Mask & static_cast<uint8>(1u << Index)) != 0;
}

bool UKzBitmaskLibrary::MarkFlagsByte(uint8& Mask, uint8 Flags)
{
	const uint8 Old = Mask;
	Mask = static_cast<uint8>(Mask | Flags);
	return Mask != Old;
}

bool UKzBitmaskLibrary::MarkFlagAtIndexByte(uint8& Mask, int32 Index)
{
	if (Index < 0 || Index >= 8) { return false; }
	const uint8 Old = Mask;
	Mask = static_cast<uint8>(Mask | static_cast<uint8>(1u << Index));
	return Mask != Old;
}

bool UKzBitmaskLibrary::ClearFlagsByte(uint8& Mask, uint8 Flags)
{
	const uint8 Old = Mask;
	Mask = static_cast<uint8>(Mask & ~Flags);
	return Mask != Old;
}

bool UKzBitmaskLibrary::ClearFlagAtIndexByte(uint8& Mask, int32 Index)
{
	if (Index < 0 || Index >= 8) { return false; }
	const uint8 Old = Mask;
	Mask = static_cast<uint8>(Mask & ~static_cast<uint8>(1u << Index));
	return Mask != Old;
}

uint8 UKzBitmaskLibrary::SetFlagAtIndexByte(uint8 Mask, int32 Index, bool bSet)
{
	if (Index < 0 || Index >= 8) { return Mask; }
	const uint8 Bit = static_cast<uint8>(1u << Index);
	return bSet ? static_cast<uint8>(Mask | Bit) : static_cast<uint8>(Mask & ~Bit);
}

uint8 UKzBitmaskLibrary::RemoveFlagsByte(uint8 Mask, uint8 Flags)
{
	return static_cast<uint8>(Mask & ~Flags);
}

int32 UKzBitmaskLibrary::CountSetFlagsByte(uint8 Mask)
{
	return FMath::CountBits(static_cast<uint64>(Mask));
}

// === Byte (uint8) bitwise operators ===

uint8 UKzBitmaskLibrary::OrByte(uint8 A, uint8 B)
{
	return static_cast<uint8>(A | B);
}

uint8 UKzBitmaskLibrary::AndByte(uint8 A, uint8 B)
{
	return static_cast<uint8>(A & B);
}

uint8 UKzBitmaskLibrary::XorByte(uint8 A, uint8 B)
{
	return static_cast<uint8>(A ^ B);
}

uint8 UKzBitmaskLibrary::NotByte(uint8 A)
{
	return static_cast<uint8>(~A);
}

// === Byte (uint8) shifts ===

uint8 UKzBitmaskLibrary::ShiftLeftByte(uint8 Value, int32 Count)
{
	if (Count <= 0) { return Value; }
	if (Count >= 8) { return 0; }
	// Bits shifted past bit 7 are dropped (mask back to a byte).
	return static_cast<uint8>((static_cast<uint32>(Value) << Count) & 0xFFu);
}

uint8 UKzBitmaskLibrary::ShiftRightByte(uint8 Value, int32 Count)
{
	if (Count <= 0) { return Value; }
	if (Count >= 8) { return 0; }
	return static_cast<uint8>(Value >> Count); // unsigned -> already zero-filled
}