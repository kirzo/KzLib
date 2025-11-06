// Copyright 2025 kirzo

#pragma once

#include "KzLibMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KzMathLibrary.generated.h"

// Whether to inline functions at all
#define KZ_KISMET_MATH_INLINE_ENABLED	(!UE_BUILD_DEBUG)

struct FKzShapeInstance;

UCLASS(meta = (BlueprintThreadSafe, ScriptName = "KzMathLibrary"))
class KZLIB_API UKzMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// === FVector ===

	/** Constructs a new vector accumulator from an array of vectors. */
	UFUNCTION(BlueprintPure, Category = "Math|Vector", meta = (Keywords = "construct build", NativeMakeFunc))
	static FKzVectorAccumulator MakeVectorAccumulator(const TArray<FVector>& Vectors);

	/** Adds a vector to the running accumulator with an optional weight. */
	UFUNCTION(BlueprintCallable, Category = "Math|Vector")
	static void AddVector(UPARAM(ref) FKzVectorAccumulator& Accumulator, const FVector& Vector, float Weight = 1.0f);

	/** Adds multiple vectors to the accumulator, each with unit weight. */
	UFUNCTION(BlueprintCallable, Category = "Math|Vector")
	static void AppendVectors(UPARAM(ref) FKzVectorAccumulator& Accumulator, const TArray<FVector>& Vectors);

	/** Adds multiple vectors with corresponding weights. Missing weights default to 1. */
	UFUNCTION(BlueprintCallable, Category = "Math|Vector")
	static void AppendVectorsWeighted(UPARAM(ref) FKzVectorAccumulator& Accumulator, const TArray<FVector>& Vectors, const TArray<float>& Weights);

	/** Returns the average accumulated vector. */
	UFUNCTION(BlueprintPure, Category = "Math|Vector")
	static FVector GetAverageVector(const FKzVectorAccumulator& Accumulator);

	/** Converts an FKzVectorAccumulator to a Vector */
	UFUNCTION(BlueprintPure, Category = "Math|Vector", meta = (DisplayName = "To Vector (FKzVectorAccumulator)", CompactNodeTitle = "->", ScriptMethod = "Vector", Keywords = "cast convert", BlueprintAutocast))
	static FVector Conv_VectorAccumulatorToVector(const FKzVectorAccumulator& Accumulator);

	/** Resets the vector accumulator. */
	UFUNCTION(BlueprintCallable, Category = "Math|Vector")
	static void ResetVectorAccumulator(UPARAM(ref) FKzVectorAccumulator& Accumulator);

	// === FQuat ===

	/** Constructs a new quaternion accumulator from an array of quaternions. */
	UFUNCTION(BlueprintPure, Category = "Math|Quat")
	static FKzQuatAccumulator MakeQuatAccumulator(const TArray<FQuat>& Quats);

	/** Adds a quaternion to the running accumulator with an optional weight. */
	UFUNCTION(BlueprintCallable, Category = "Math|Quat")
	static void AddQuat(UPARAM(ref) FKzQuatAccumulator& Accumulator, const FQuat& Quat, float Weight = 1.0f);

	/** Adds multiple quaternions to the accumulator, each with unit weight. */
	UFUNCTION(BlueprintCallable, Category = "Math|Quat")
	static void AppendQuats(UPARAM(ref) FKzQuatAccumulator& Accumulator, const TArray<FQuat>& Quats);

	/** Adds multiple quaternions with corresponding weights. Missing weights default to 1. */
	UFUNCTION(BlueprintCallable, Category = "Math|Quat")
	static void AppendQuatsWeighted(UPARAM(ref) FKzQuatAccumulator& Accumulator, const TArray<FQuat>& Quats, const TArray<float>& Weights);

	/** Returns the normalized average accumulated quaternion. */
	UFUNCTION(BlueprintPure, Category = "Math|Quat")
	static FQuat GetAverageQuat(const FKzQuatAccumulator& Accumulator);

	/** Converts an FKzQuatAccumulator to a Quat */
	UFUNCTION(BlueprintPure, Category = "Math|Quat", meta = (DisplayName = "To Quat (FKzQuatAccumulator)", CompactNodeTitle = "->", ScriptMethod = "Quat", Keywords = "cast convert", BlueprintAutocast))
	static FQuat Conv_QuatAccumulatorToQuat(const FKzQuatAccumulator& Accumulator);

	/** Resets the quaternion accumulator. */
	UFUNCTION(BlueprintCallable, Category = "Math|Quat")
	static void ResetQuatAccumulator(UPARAM(ref) FKzQuatAccumulator& Accumulator);

	// === FRotator ===

	/** Constructs a new quaternion accumulator from an array of rotators. */
	UFUNCTION(BlueprintPure, Category = "Math|Rotator", meta = (Keywords = "construct build", NativeMakeFunc))
	static FKzQuatAccumulator MakeRotatorAccumulator(const TArray<FRotator>& Rotations);

	/** Adds a rotator to the running accumulator with an optional weight. */
	UFUNCTION(BlueprintCallable, Category = "Math|Rotator")
	static void AddRotator(UPARAM(ref) FKzQuatAccumulator& Accumulator, const FRotator& Rotation, float Weight = 1.0f);

	/** Adds multiple rotators to the accumulator, each with unit weight. */
	UFUNCTION(BlueprintCallable, Category = "Math|Rotator")
	static void AppendRotators(UPARAM(ref) FKzQuatAccumulator& Accumulator, const TArray<FRotator>& Rotations);

	/** Adds multiple rotators with corresponding weights. Missing weights default to 1. */
	UFUNCTION(BlueprintCallable, Category = "Math|Rotator")
	static void AppendRotatorsWeighted(UPARAM(ref) FKzQuatAccumulator& Accumulator, const TArray<FRotator>& Rotations, const TArray<float>& Weights);

	/** Returns the average accumulated quaternion as a rotator. */
	UFUNCTION(BlueprintPure, Category = "Math|Rotator")
	static FRotator GetAverageRotator(const FKzQuatAccumulator& Accumulator);

	/** Converts an FKzQuatAccumulator to a Rotator */
	UFUNCTION(BlueprintPure, Category = "Math|Rotator", meta = (DisplayName = "To Rotator (FKzQuatAccumulator)", CompactNodeTitle = "->", ScriptMethod = "Rotator", Keywords = "cast convert", BlueprintAutocast))
	static FRotator Conv_QuatAccumulatorToRotator(const FKzQuatAccumulator& Accumulator);
};

// Inline implementations
#if KZ_KISMET_MATH_INLINE_ENABLED
#include "KzMathLibrary.inl"
#endif