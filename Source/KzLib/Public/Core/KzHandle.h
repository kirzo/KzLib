// Copyright 2026 kirzo

#pragma once

#include "KzHandle.generated.h"

/**
 * Lightweight, generic handle for dense container systems.
 * Designed to be a safe, weak reference to an object that may move in memory.
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzHandle
{
	GENERATED_BODY()

	/** Invalid index constant */
	static constexpr int32 InvalidIndex = INDEX_NONE;

	/** Index into the dense array / lookup table */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handle")
	int32 Index = InvalidIndex;

	/** Generation ID to detect stale references (ABA problem) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handle")
	int32 Generation = 0;

	/** Default constructor (Invalid) */
	FKzHandle() = default;

	/** Fast constructor */
	FKzHandle(int32 InIndex, int32 InGeneration)
		: Index(InIndex), Generation(InGeneration)
	{
	}

	/** Copy constructor to allow safe inheritance. */
	FKzHandle(const FKzHandle&) = default;

	/** Validity Check */
	FORCEINLINE bool IsValid() const
	{
		return Index != InvalidIndex;
	}

	/** Explicit invalidation */
	void Invalidate()
	{
		Index = InvalidIndex;
		Generation = 0;
	}

	FORCEINLINE bool operator==(const FKzHandle& Other) const
	{
		return Index == Other.Index && Generation == Other.Generation;
	}

	FORCEINLINE bool operator!=(const FKzHandle& Other) const
	{
		return !(*this == Other);
	}

	/** Conversion to Bool for simple "if (Handle)" checks */
	explicit operator bool() const
	{
		return IsValid();
	}

	friend uint32 GetTypeHash(const FKzHandle& Handle)
	{
		return HashCombineFast(::GetTypeHash(Handle.Index), ::GetTypeHash(Handle.Generation));
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("[%d:%d]"), Index, Generation);
	}
};