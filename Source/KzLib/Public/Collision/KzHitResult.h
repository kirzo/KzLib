// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "KzHitResult.generated.h"

/** A struct to hold the comprehensive results of a raycast. */
USTRUCT(BlueprintType, meta = (HasNativeBreak = "/Script/KzLib.KzSystemLibrary.BreakHitResult"))
struct FKzHitResult
{
	GENERATED_BODY()

	/** True if the ray hit something, false otherwise. */
	UPROPERTY()
	bool bBlockingHit = false;

	/** Whether the trace started in penetration, i.e. with an initial blocking overlap. */
	UPROPERTY()
	uint8 bStartPenetrating : 1;

	/** 'Time' of impact along trace direction (ranging from 0.0 to 1.0) if there is a hit, indicating time between TraceStart and TraceEnd. */
	UPROPERTY()
	float Time;

	/** The distance from the ray's origin to the point of impact. */
	UPROPERTY()
	float Distance = 0.0f;

	/** The world-space location where the ray impacted. */
	UPROPERTY()
	FVector_NetQuantize Location = FVector::ZeroVector;

	/** The world-space normal where the ray impacted. */
	UPROPERTY()
	FVector_NetQuantizeNormal Normal;

	/** Start location of the trace. */
	UPROPERTY()
	FVector_NetQuantize TraceStart;

	/** End location of the trace; this is NOT where the impact occurred (if any), but the furthest point in the attempted sweep. */
	UPROPERTY()
	FVector_NetQuantize TraceEnd;

	FKzHitResult()
	{
		Init();
	}

	explicit FKzHitResult(float InTime)
	{
		Init();
		Time = InTime;
	}

	explicit FKzHitResult(EForceInit InInit)
	{
		Init();
	}

	explicit FKzHitResult(ENoInit NoInit)
	{
	}

	explicit FKzHitResult(FVector Start, FVector End)
	{
		Init(Start, End);
	}

	/** Initialize empty hit result with given time. */
	FORCEINLINE void Init()
	{
		FMemory::Memzero(this, sizeof(FKzHitResult));
		Time = 1.0f;
	}

	/** Initialize empty hit result with given time, TraceStart, and TraceEnd */
	FORCEINLINE void Init(FVector Start, FVector End)
	{
		Init();
		TraceStart = Start;
		TraceEnd = End;
	}

	/** Reset hit result while optionally saving TraceStart and TraceEnd. */
	FORCEINLINE void Reset(float InTime = 1.0f, bool bPreserveTraceData = true)
	{
		const FVector SavedTraceStart = TraceStart;
		const FVector SavedTraceEnd = TraceEnd;
		Init();
		Time = InTime;
		if (bPreserveTraceData)
		{
			TraceStart = SavedTraceStart;
			TraceEnd = SavedTraceEnd;
		}
	}

	/** Optimized serialize function */
	KZLIB_API bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	/** Return true if there was a blocking hit that was not caused by starting in penetration. */
	FORCEINLINE bool IsValidBlockingHit() const
	{
		return bBlockingHit && !bStartPenetrating;
	}

	/** Static utility function that returns the first 'blocking' hit in an array of results. */
	static FKzHitResult* GetFirstBlockingHit(TArray<FKzHitResult>& InHits)
	{
		for (int32 HitIdx = 0; HitIdx < InHits.Num(); HitIdx++)
		{
			if (InHits[HitIdx].bBlockingHit)
			{
				return &InHits[HitIdx];
			}
		}
		return nullptr;
	}

	/** Static utility function that returns the number of blocking hits in array. */
	static int32 GetNumBlockingHits(const TArray<FKzHitResult>& InHits)
	{
		int32 NumBlocks = 0;
		for (int32 HitIdx = 0; HitIdx < InHits.Num(); HitIdx++)
		{
			if (InHits[HitIdx].bBlockingHit)
			{
				NumBlocks++;
			}
		}
		return NumBlocks;
	}

	/** Static utility function that returns the number of overlapping hits in array. */
	static int32 GetNumOverlapHits(const TArray<FKzHitResult>& InHits)
	{
		return (InHits.Num() - GetNumBlockingHits(InHits));
	}

	/**
	 * Get a copy of the HitResult with relevant information reversed.
	 * For example when receiving a hit from another object, we reverse the normals.
	 */
	static FKzHitResult GetReversedHit(const FKzHitResult& Hit)
	{
		FKzHitResult Result(Hit);
		Result.Normal = -Result.Normal;
		return Result;
	}

	KZLIB_API FString ToString() const;

	KZLIB_API struct FHitResult ToHitResult() const;
};

template<>
struct TStructOpsTypeTraits<FKzHitResult> : public TStructOpsTypeTraitsBase2<FKzHitResult>
{
	enum
	{
		WithNetSerializer = true,
	};
};