// Copyright 2025 kirzo

#pragma once

#include "Math/Vector.h"
#include "KzVectorAccumulator.generated.h"

/**
 * Helper struct for computing the average of multiple vectors.
 * Supports weighted accumulation.
 */
USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/KzLib.KzMathLibrary.MakeVectorAccumulator"))
struct FKzVectorAccumulator
{
	GENERATED_BODY()

	/** Returns true if at least one vector has been added. */
	bool IsValid() const
	{
		return TotalWeight > 0.0f;
	}

	/** Resets the accumulator. */
	void Reset()
	{
		Accumulated = FVector::ZeroVector;
		TotalWeight = 0.0f;
	}

	/** Returns the normalized average vector. */
	FVector Get() const
	{
		return IsValid() ? Accumulated / TotalWeight : FVector::ZeroVector;
	}

	/** Returns the total accumulated weight. */
	float GetTotalWeight() const
	{
		return TotalWeight;
	}

	FKzVectorAccumulator() = default;

	FKzVectorAccumulator(EForceInit)
	{
	}

	/** Construct from a single vector. */
	FKzVectorAccumulator(const FVector& Vector)
	{
		AddWeighted(Vector);
	}

	/** Construct from a list of vectors. */
	FKzVectorAccumulator(std::initializer_list<FVector> Vectors)
	{
		for (const FVector& V : Vectors)
		{
			AddWeighted(V);
		}
	}

	/** Construct from an array of vectors. */
	explicit FKzVectorAccumulator(const TArrayView<const FVector> Vectors)
	{
		for (const FVector& V : Vectors)
		{
			AddWeighted(V);
		}
	}

	/** Adds a weighted vector to the running average. */
	void AddWeighted(const FVector& VectorIn, float Weight = 1.0f)
	{
		if (Weight <= 0.0f)
		{
			return; // Ignore zero or negative weights.
		}

		Accumulated += VectorIn * Weight;
		TotalWeight += Weight;
	}

	/** Adds multiple vectors to the running average, each with unit weight. */
	void Append(const TArrayView<const FVector> Vectors)
	{
		for (const FVector& V : Vectors)
		{
			AddWeighted(V);
		}
	}

	/** Adds multiple vectors with individual weights. */
	void AppendWeighted(const TArrayView<const FVector> Vectors, const TArrayView<const float> Weights)
	{
		if (Vectors.Num() != Weights.Num())
		{
			ensureMsgf(false, TEXT("AppendWeighted: Vectors and Weights size mismatch (%d vs %d). Missing weights default to 1."), Vectors.Num(), Weights.Num());
		}

		const int32 Count = Vectors.Num();
		for (int32 i = 0; i < Count; ++i)
		{
			const float Weight = (i < Weights.Num()) ? Weights[i] : 1.0f;
			AddWeighted(Vectors[i], Weight);
		}
	}

	/**
		* Check against another vector accumulator for equality.
		*
		* @param V The vector accumulator to check against.
		* @return true if the accumulators are equal, false otherwise.
		*/
	bool operator==(const FKzVectorAccumulator& V) const
	{
		return Get() == V.Get();
	}

	/**
		* Check against another vector accumulator for inequality.
		*
		* @param V The vector accumulator to check against.
		* @return true if the accumulators are not equal, false otherwise.
		*/
	bool operator!=(const FKzVectorAccumulator& V) const
	{
		return Get() != V.Get();
	}

	/** Adds a vector with unit weight (equivalent to AddWeighted with weight = 1). */
	FKzVectorAccumulator& operator+=(const FVector& Vector)
	{
		AddWeighted(Vector);
		return *this;
	}

	/** Allows implicit conversion to TVector<T>, returning the averaged vector. */
	operator FVector() const { return Get(); }

private:
	/** Accumulated vector sum for averaging. */
	FVector Accumulated = FVector::ZeroVector;

	/** Total accumulated weight (or count, if unweighted). */
	float TotalWeight = 0.0f;
};