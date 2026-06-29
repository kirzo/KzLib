// Copyright 2026 kirzo

#pragma once

#include "KzLibMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KzBitmaskLibrary.generated.h"

/**
 * Blueprint helpers for bit flags / bitmasks. Unreal already exposes the integer bitwise operators
 * (AND/OR/XOR/NOT), so this covers what it doesn't: index-driven flags (the shift is done for you),
 * named flag convenience (contains / mark / clear, with change detection), and the same kit for byte
 * (uint8), whose bitwise operators Blueprint lacks entirely.
 *
 * "Flag" is a value with its bit(s) set (e.g. 4); "Index" is a bit position (e.g. 2 -> 1<<2 == 4).
 */
UCLASS(meta = (BlueprintThreadSafe, ScriptName = "KzBitmaskLibrary"))
class KZLIB_API UKzBitmaskLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// === Integer flags ===

	/** Flag value for a bit position: 1 << Index (Index 0..31; out of range -> 0). The "automatic shift". */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (Keywords = "bit shift flag mask power of two"))
	static int32 MakeFlag(int32 Index);

	/** True if Mask contains every bit of Flag, i.e. (Mask & Flag) == Flag. */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask has all contains"))
	static bool ContainsFlag(int32 Mask, int32 Flag);

	/** True if Mask contains any bit of Flags, i.e. (Mask & Flags) != 0. */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask has any contains"))
	static bool ContainsAnyFlags(int32 Mask, int32 Flags);

	/** True if Mask contains every bit of Flags, i.e. (Mask & Flags) == Flags (Flags == 0 -> false). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask has all contains every"))
	static bool ContainsAllFlags(int32 Mask, int32 Flags);

	/** True if the bit at Index is set (Index 0..31). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask has index contains"))
	static bool ContainsFlagAtIndex(int32 Mask, int32 Index);

	/** Sets Flags on Mask (Mask |= Flags). Returns true if it changed Mask (some bit went 0->1). */
	UFUNCTION(BlueprintCallable, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask set or add mark"))
	static bool MarkFlags(UPARAM(ref) int32& Mask, int32 Flags);

	/** Sets the bit at Index on Mask. Returns true if it went 0->1 (Index 0..31). */
	UFUNCTION(BlueprintCallable, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask set index mark"))
	static bool MarkFlagAtIndex(UPARAM(ref) int32& Mask, int32 Index);

	/** Clears Flags from Mask (Mask &= ~Flags). Returns true if it changed Mask (some bit went 1->0). */
	UFUNCTION(BlueprintCallable, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask clear remove unset"))
	static bool ClearFlags(UPARAM(ref) int32& Mask, int32 Flags);

	/** Clears the bit at Index on Mask. Returns true if it went 1->0 (Index 0..31). */
	UFUNCTION(BlueprintCallable, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask clear index unset"))
	static bool ClearFlagAtIndex(UPARAM(ref) int32& Mask, int32 Index);

	/** Returns Mask with the bit at Index set or cleared (pure; leaves the input untouched). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask set clear index"))
	static int32 SetFlagAtIndex(int32 Mask, int32 Index, bool bSet);

	/** Returns Mask with Flags removed (Mask & ~Flags) (pure). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask remove clear and not"))
	static int32 RemoveFlags(int32 Mask, int32 Flags);

	/** Number of set bits in Mask (popcount) — e.g. how many flags are on. */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (Keywords = "bit flag mask count popcount ones"))
	static int32 CountSetFlags(int32 Mask);

	// === Integer shifts (Blueprint ships no shift node) ===

	/** Value shifted left by Count bits (Value << Count). Count <= 0 leaves it unchanged, >= 32 gives 0. */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (DisplayName = "Shift Left", CompactNodeTitle = "<<", Keywords = "bit shift left"))
	static int32 ShiftLeft(int32 Value, int32 Count);

	/** Value shifted right by Count bits, zero-filled / logical (Value >> Count). Count <= 0 leaves it unchanged, >= 32 gives 0. */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Integer", meta = (DisplayName = "Shift Right", CompactNodeTitle = ">>", Keywords = "bit shift right logical zero fill"))
	static int32 ShiftRight(int32 Value, int32 Count);

	// === Byte (uint8) flags ===

	/** Flag value for a bit position in a byte: 1 << Index (Index 0..7; out of range -> 0). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Make Flag (Byte)", Keywords = "bit shift flag mask byte"))
	static uint8 MakeFlagByte(int32 Index);

	/** True if Mask contains every bit of Flag, i.e. (Mask & Flag) == Flag. */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Contains Flag (Byte)", Keywords = "bit flag mask has all contains byte"))
	static bool ContainsFlagByte(uint8 Mask, uint8 Flag);

	/** True if Mask contains any bit of Flags, i.e. (Mask & Flags) != 0. */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Contains Any Flags (Byte)", Keywords = "bit flag mask has any contains byte"))
	static bool ContainsAnyFlagsByte(uint8 Mask, uint8 Flags);

	/** True if Mask contains every bit of Flags, i.e. (Mask & Flags) == Flags (Flags == 0 -> false). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Contains All Flags (Byte)", Keywords = "bit flag mask has all contains every byte"))
	static bool ContainsAllFlagsByte(uint8 Mask, uint8 Flags);

	/** True if the bit at Index is set (Index 0..7). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Contains Flag At Index (Byte)", Keywords = "bit flag mask has index contains byte"))
	static bool ContainsFlagAtIndexByte(uint8 Mask, int32 Index);

	/** Sets Flags on Mask (Mask |= Flags). Returns true if it changed Mask (some bit went 0->1). */
	UFUNCTION(BlueprintCallable, Category = "Bitmask|Byte", meta = (DisplayName = "Mark Flags (Byte)", Keywords = "bit flag mask set or add mark byte"))
	static bool MarkFlagsByte(UPARAM(ref) uint8& Mask, uint8 Flags);

	/** Sets the bit at Index on Mask. Returns true if it went 0->1 (Index 0..7). */
	UFUNCTION(BlueprintCallable, Category = "Bitmask|Byte", meta = (DisplayName = "Mark Flag At Index (Byte)", Keywords = "bit flag mask set index mark byte"))
	static bool MarkFlagAtIndexByte(UPARAM(ref) uint8& Mask, int32 Index);

	/** Clears Flags from Mask (Mask &= ~Flags). Returns true if it changed Mask (some bit went 1->0). */
	UFUNCTION(BlueprintCallable, Category = "Bitmask|Byte", meta = (DisplayName = "Clear Flags (Byte)", Keywords = "bit flag mask clear remove unset byte"))
	static bool ClearFlagsByte(UPARAM(ref) uint8& Mask, uint8 Flags);

	/** Clears the bit at Index on Mask. Returns true if it went 1->0 (Index 0..7). */
	UFUNCTION(BlueprintCallable, Category = "Bitmask|Byte", meta = (DisplayName = "Clear Flag At Index (Byte)", Keywords = "bit flag mask clear index unset byte"))
	static bool ClearFlagAtIndexByte(UPARAM(ref) uint8& Mask, int32 Index);

	/** Returns Mask with the bit at Index set or cleared (pure; leaves the input untouched). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Set Flag At Index (Byte)", Keywords = "bit flag mask set clear index byte"))
	static uint8 SetFlagAtIndexByte(uint8 Mask, int32 Index, bool bSet);

	/** Returns Mask with Flags removed (Mask & ~Flags) (pure). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Remove Flags (Byte)", Keywords = "bit flag mask remove clear and not byte"))
	static uint8 RemoveFlagsByte(uint8 Mask, uint8 Flags);

	/** Number of set bits in Mask (popcount). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Count Set Flags (Byte)", Keywords = "bit flag mask count popcount ones byte"))
	static int32 CountSetFlagsByte(uint8 Mask);

	// === Byte (uint8) bitwise operators (Blueprint ships these for int, not for byte) ===

	/** Bitwise OR of two bytes (A | B). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Bitwise OR (Byte)", CompactNodeTitle = "|", Keywords = "bitwise or byte | flag"))
	static uint8 OrByte(uint8 A, uint8 B);

	/** Bitwise AND of two bytes (A & B). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Bitwise AND (Byte)", CompactNodeTitle = "&", Keywords = "bitwise and byte & flag"))
	static uint8 AndByte(uint8 A, uint8 B);

	/** Bitwise XOR of two bytes (A ^ B). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Bitwise XOR (Byte)", CompactNodeTitle = "^", Keywords = "bitwise xor byte ^ flag toggle"))
	static uint8 XorByte(uint8 A, uint8 B);

	/** Bitwise NOT of a byte (~A). */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Bitwise NOT (Byte)", CompactNodeTitle = "~", Keywords = "bitwise not byte ~ flag complement invert"))
	static uint8 NotByte(uint8 A);

	/** Byte shifted left by Count bits (bits past bit 7 are dropped). Count <= 0 leaves it unchanged, >= 8 gives 0. */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Shift Left (Byte)", CompactNodeTitle = "<<", Keywords = "bit shift left byte"))
	static uint8 ShiftLeftByte(uint8 Value, int32 Count);

	/** Byte shifted right by Count bits, zero-filled (Value >> Count). Count <= 0 leaves it unchanged, >= 8 gives 0. */
	UFUNCTION(BlueprintPure, Category = "Bitmask|Byte", meta = (DisplayName = "Shift Right (Byte)", CompactNodeTitle = ">>", Keywords = "bit shift right byte logical zero fill"))
	static uint8 ShiftRightByte(uint8 Value, int32 Count);
};