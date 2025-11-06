// Copyright 2025 kirzo

#pragma once

#include "KzMathLibrary.h"

#if KZ_KISMET_MATH_INLINE_ENABLED
#define KZ_MATH_FORCEINLINE FORCEINLINE
#else
#define KZ_MATH_FORCEINLINE // nothing
#endif

KZ_MATH_FORCEINLINE
static TArray<FQuat> ConvertRotatorsToQuats(const TArray<FRotator>& Rotators)
{
	TArray<FQuat> Quats;
	Quats.Reserve(Rotators.Num());

	for (const FRotator& Rot : Rotators)
	{
		Quats.Add(Rot.Quaternion());
	}

	return Quats;
}

// === FVector ===

KZ_MATH_FORCEINLINE
FKzVectorAccumulator UKzMathLibrary::MakeVectorAccumulator(const TArray<FVector>& Vectors)
{
	return FKzVectorAccumulator(MakeArrayView(Vectors));
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::AddVector(UPARAM(ref)FKzVectorAccumulator& Accumulator, const FVector& Vector, float Weight)
{
	Accumulator.AddWeighted(Vector, Weight);
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::AppendVectors(UPARAM(ref)FKzVectorAccumulator& Accumulator, const TArray<FVector>& Vectors)
{
	Accumulator.Append(MakeArrayView(Vectors));
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::AppendVectorsWeighted(UPARAM(ref)FKzVectorAccumulator& Accumulator, const TArray<FVector>& Vectors, const TArray<float>& Weights)
{
	Accumulator.AppendWeighted(MakeArrayView(Vectors), MakeArrayView(Weights));
}

KZ_MATH_FORCEINLINE
FVector UKzMathLibrary::GetAverageVector(const FKzVectorAccumulator& Accumulator)
{
	return Accumulator;
}

KZ_MATH_FORCEINLINE
FVector UKzMathLibrary::Conv_VectorAccumulatorToVector(const FKzVectorAccumulator& Accumulator)
{
	return Accumulator;
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::ResetVectorAccumulator(UPARAM(ref)FKzVectorAccumulator& Accumulator)
{
	Accumulator.Reset();
}

// === FQuat ===

KZ_MATH_FORCEINLINE
FKzQuatAccumulator UKzMathLibrary::MakeQuatAccumulator(const TArray<FQuat>& Quats)
{
	return FKzQuatAccumulator(MakeArrayView(Quats));
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::AddQuat(FKzQuatAccumulator& Accumulator, const FQuat& Quat, float Weight)
{
	Accumulator.AddWeighted(Quat, Weight);
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::AppendQuats(FKzQuatAccumulator& Accumulator, const TArray<FQuat>& Quats)
{
	Accumulator.Append(MakeArrayView(Quats));
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::AppendQuatsWeighted(FKzQuatAccumulator& Accumulator, const TArray<FQuat>& Quats, const TArray<float>& Weights)
{
	Accumulator.AppendWeighted(MakeArrayView(Quats), MakeArrayView(Weights));
}

KZ_MATH_FORCEINLINE
FQuat UKzMathLibrary::GetAverageQuat(const FKzQuatAccumulator& Accumulator)
{
	return Accumulator;
}

inline FQuat UKzMathLibrary::Conv_QuatAccumulatorToQuat(const FKzQuatAccumulator& Accumulator)
{
	return Accumulator;
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::ResetQuatAccumulator(FKzQuatAccumulator& Accumulator)
{
	Accumulator.Reset();
}

// === FRotator ===

KZ_MATH_FORCEINLINE
FKzQuatAccumulator UKzMathLibrary::MakeRotatorAccumulator(const TArray<FRotator>& Rotations)
{
	return MakeQuatAccumulator(ConvertRotatorsToQuats(Rotations));
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::AddRotator(FKzQuatAccumulator& Accumulator, const FRotator& Rotation, float Weight)
{
	Accumulator.AddWeighted(Rotation.Quaternion(), Weight);
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::AppendRotators(FKzQuatAccumulator& Accumulator, const TArray<FRotator>& Rotations)
{
	AppendQuats(Accumulator, ConvertRotatorsToQuats(Rotations));
}

KZ_MATH_FORCEINLINE
void UKzMathLibrary::AppendRotatorsWeighted(FKzQuatAccumulator& Accumulator, const TArray<FRotator>& Rotations, const TArray<float>& Weights)
{
	AppendQuatsWeighted(Accumulator, ConvertRotatorsToQuats(Rotations), Weights);
}

KZ_MATH_FORCEINLINE
FRotator UKzMathLibrary::GetAverageRotator(const FKzQuatAccumulator& Accumulator)
{
	return Accumulator.Get().Rotator();
}

KZ_MATH_FORCEINLINE
FRotator UKzMathLibrary::Conv_QuatAccumulatorToRotator(const FKzQuatAccumulator& Accumulator)
{
	return Accumulator.Get().Rotator();
}

#undef KZ_MATH_FORCEINLINE