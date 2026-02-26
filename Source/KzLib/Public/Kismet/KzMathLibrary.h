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

	// === Math ===

	/**
	 * Calculates the horizontal angle (yaw) between two direction vectors.
	 * Returns the angle in degrees (0 to 180).
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Vector", meta = (DisplayName = "Get Horizontal Angle"))
	static float GetHorizontalAngle(const FVector& A, const FVector& B);

	/**
	 * Calculates the absolute difference in vertical angle between two direction vectors.
	 * Returns the difference in degrees.
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Vector", meta = (DisplayName = "Get Vertical Angle Difference"))
	static float GetVerticalAngleDifference(const FVector& A, const FVector& B);

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

	/**
	 * Converts a Quaternion into a Rotation Vector (Axis * Angle).
	 * Ideally used for Angular Velocity targets or Torque calculations.
	 * Uses the Quaternion Logarithm for optimized shortest-path calculation.
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Quat", meta = (DisplayName = "To Rotation Vector (Quat)"))
	static FVector QuatToRotationVector(const FQuat& Quat);

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

	/**
	 * Converts a Rotator into a Rotation Vector (Axis * Angle).
	 * Ideally used for Angular Velocity targets or Torque calculations.
	 * Uses the Quaternion Logarithm for optimized shortest-path calculation.
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Quat", meta = (DisplayName = "To Rotation Vector (FRotator)"))
	static FVector RotatorToRotationVector(const FRotator& Rotation);

	// === Transform ===

	/**
	 * Transforms a point from local space (relative to ParentPosition/ParentRotation) to world space.
	 * Equivalent to: ParentTransform.TransformPosition(LocalPoint).
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Transform", meta = (Keywords = "location local to world relative"))
	static FVector TransformLocation(const FVector ParentPosition, const FRotator ParentRotation, const FVector LocalPoint);

	/**
	 * Transforms a point from world space to local space (relative to ParentPosition/ParentRotation).
	 * Equivalent to: ParentTransform.InverseTransformPosition(WorldPoint).
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Transform", meta = (Keywords = "location world to local inverse relative"))
	static FVector InverseTransformLocation(const FVector ParentPosition, const FRotator ParentRotation, const FVector WorldPoint);

	/**
	 * Transforms a point from local space (relative to ParentPosition/ParentRotation) to world space.
	 * Equivalent to: ParentTransform.TransformPosition(LocalPoint).
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Transform", meta = (Keywords = "location local to world relative", DisplayName = "Transform Location (Quat)"))
	static FVector TransformLocation_Quat(const FVector ParentPosition, const FQuat ParentRotation, const FVector LocalPoint);

	/**
	 * Transforms a point from world space to local space (relative to ParentPosition/ParentRotation).
	 * Equivalent to: ParentTransform.InverseTransformPosition(WorldPoint).
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Transform", meta = (Keywords = "location world to local inverse relative", DisplayName = "Inverse Transform Location (Quat)"))
	static FVector InverseTransformLocation_Quat(const FVector ParentPosition, const FQuat ParentRotation, const FVector WorldPoint);
};

// Inline implementations
#if KZ_KISMET_MATH_INLINE_ENABLED
#include "KzMathLibrary.inl"
#endif