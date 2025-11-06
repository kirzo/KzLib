// Copyright 2025 kirzo

#pragma once

#include "Math/Quat.h"
#include "KzQuatAccumulator.generated.h"

/**
 * Helper struct for computing the average of multiple quaternions.
 * Supports weighted accumulation.
 */
USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/KzLib.KzMathLibrary.MakeRotatorAccumulator"))
struct FKzQuatAccumulator
{
	GENERATED_BODY()

	/** Returns true if at least one quaternion has been added. */
	bool IsValid() const
	{
		return TotalWeight > 0.0f;
	}

	/** Resets the accumulator. */
	void Reset()
	{
		Accumulated = FQuat::Identity;
		FirstQuat = FQuat::Identity;
		TotalWeight = 0.0f;
	}

	/** Returns the normalized average quaternion. */
	FQuat Get() const
	{
		return IsValid() ? Accumulated.GetNormalized() : FQuat::Identity;
	}

	/** Returns the total accumulated weight. */
	float GetTotalWeight() const
	{
		return TotalWeight;
	}

	FKzQuatAccumulator() = default;

	FKzQuatAccumulator(EForceInit)
	{
	}

	/** Construct from a single quaternion. */
	FKzQuatAccumulator(const FQuat& Quat)
	{
		AddWeighted(Quat);
	}

	/** Construct from a list of quaternions. */
	FKzQuatAccumulator(std::initializer_list<FQuat> Quats)
	{
		for (const FQuat& Q : Quats)
		{
			AddWeighted(Q);
		}
	}

	/** Construct from an array of quaternions. */
	explicit FKzQuatAccumulator(const TArrayView<const FQuat> Quats)
	{
		for (const FQuat& Q : Quats)
		{
			AddWeighted(Q, 1);
		}
	}

	/** Adds a weighted quaternion to the running average. */
	void AddWeighted(const FQuat& QuatIn, float Weight = 1.0f)
	{
		if (Weight <= 0.0f)
		{
			return; // Ignore zero or negative weights.
		}

		FQuat Quat = QuatIn;

		if (TotalWeight == 0.0f)
		{
			// First quaternion establishes the reference orientation.
			FirstQuat = Quat;
			Accumulated = Quat * Weight;
		}
		else
		{
			// Ensure the quaternion lies in the same hemisphere as the reference.
			if ((FirstQuat | Quat) < 0.0f)
			{
				Quat = -Quat;
			}

			Accumulated += Quat * Weight;
		}

		TotalWeight += Weight;
	}

	/** Adds multiple quaternions to the running average, each with unit weight. */
	void Append(const TArrayView<const FQuat> Quats)
	{
		for (const FQuat& Q : Quats)
		{
			AddWeighted(Q);
		}
	}

	/** Adds multiple quaternions with individual weights. */
	void AppendWeighted(const TArrayView<const FQuat> Quats, const TArrayView<const float> Weights)
	{
		if (Quats.Num() != Weights.Num())
		{
			ensureMsgf(false, TEXT("AppendWeighted: Quats and Weights size mismatch (%d vs %d). Missing weights default to 1."), Quats.Num(), Weights.Num());
		}

		const int32 Count = Quats.Num();
		for (int32 i = 0; i < Count; ++i)
		{
			const float Weight = (i < Weights.Num()) ? Weights[i] : 1.0f;
			AddWeighted(Quats[i], Weight);
		}
	}

	/**
		* Check against another quat accumulator for equality.
		*
		* @param Q The quat accumulator to check against.
		* @return true if the accumulators are equal, false otherwise.
		*/
	bool operator==(const FKzQuatAccumulator& V) const
	{
		return Get() == V.Get();
	}

	/**
		* Check against another quat accumulator for inequality.
		*
		* @param Q The quat accumulator to check against.
		* @return true if the accumulators are not equal, false otherwise.
		*/
	bool operator!=(const FKzQuatAccumulator& V) const
	{
		return Get() != V.Get();
	}

	/** Adds a quaternion with unit weight (equivalent to AddWeighted with weight = 1). */
	FKzQuatAccumulator& operator+=(const FQuat& Quat)
	{
		AddWeighted(Quat, 1);
		return *this;
	}

	/** Allows implicit conversion to FQuat, returning the averaged quaternion. */
	operator FQuat() const { return Get(); }

	bool Identical(const FKzQuatAccumulator* Q, const uint32 PortFlags) const
	{
		return Accumulated.Identical(&Q->Accumulated, PortFlags ) && FirstQuat.Identical(&Q->FirstQuat, PortFlags) && TotalWeight == Q->TotalWeight;
	}

private:
	/** Accumulated quaternion sum for averaging. */
	FQuat Accumulated = FQuat::Identity;

	/** First quaternion used as hemisphere reference. */
	FQuat FirstQuat = FQuat::Identity;

	/** Total accumulated weight (or count, if unweighted). */
	float TotalWeight = 0.0f;
};