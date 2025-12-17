// Copyright 2025 kirzo

#pragma once

#include "Math/Box.h"
#include "PhysicsEngine/BodySetup.h"

namespace Kz::Geom::Sample
{
	/** Generates the 8 corner vertices of an Axis-Aligned Bounding Box (AABB). */
	FORCEINLINE void BoxVertices(const FBox& B, TArray<FVector>& Out)
	{
		Out.Reset(8);
		const FVector Min = B.Min;
		const FVector Max = B.Max;

		const FVector C[8] =
		{
			{Min.X, Min.Y, Min.Z},
			{Max.X, Min.Y, Min.Z},
			{Min.X, Max.Y, Min.Z},
			{Max.X, Max.Y, Min.Z},
			{Min.X, Min.Y, Max.Z},
			{Max.X, Min.Y, Max.Z},
			{Min.X, Max.Y, Max.Z},
			{Max.X, Max.Y, Max.Z},
		};

		Out.Append(C, 8);
	}

	/** Generates the 8 corner vertices of an Oriented Box (Physics Element). */
	FORCEINLINE void BoxVertices(const FKBoxElem& Box, TArray<FVector>& Out)
	{
		Out.Reset(8);

		const FQuat   Q = Box.Rotation.Quaternion();
		const FVector HalfExtents = FVector(Box.X * 0.5f, Box.Y * 0.5f, Box.Z * 0.5f);

		const FVector V[8] =
		{
			{-1.f,-1.f,-1.f}, {+1.f,-1.f,-1.f},
			{-1.f,+1.f,-1.f}, {+1.f,+1.f,-1.f},
			{-1.f,-1.f,+1.f}, {+1.f,-1.f,+1.f},
			{-1.f,+1.f,+1.f}, {+1.f,+1.f,+1.f},
		};

		for (int32 i = 0; i < 8; i++)
		{
			const FVector P = V[i] * HalfExtents;
			Out.Add(Q.RotateVector(P) + Box.Center);
		}
	}

	/** Generates 6 cardinal points (extremes along X, Y, Z axes) of a Sphere Element. */
	FORCEINLINE void SphereVertices(const FKSphereElem& S, TArray<FVector>& Out)
	{
		const float R = S.Radius;

		Out.Reset(6);
		Out.Add(FVector(R, 0, 0) + S.Center);
		Out.Add(FVector(-R, 0, 0) + S.Center);
		Out.Add(FVector(0, R, 0) + S.Center);
		Out.Add(FVector(0, -R, 0) + S.Center);
		Out.Add(FVector(0, 0, R) + S.Center);
		Out.Add(FVector(0, 0, -R) + S.Center);
	}

	/** Generates 8 sample points for a Capsule (Sphyl) Element. The points are located at the connection rings where the cylinder meets the hemispheres. */
	FORCEINLINE void SphylVertices(const FKSphylElem& S, TArray<FVector>& Out)
	{
		const float R = S.Radius;
		const float H = S.Length * 0.5f;
		const FQuat Q = S.Rotation.Quaternion();

		Out.Reset(8);

		FVector P[8] =
		{
			{+R, 0.f, +H}, {-R, 0.f, +H}, {0.f, +R, +H}, {0.f, -R, +H},
			{+R, 0.f, -H}, {-R, 0.f, -H}, {0.f, +R, -H}, {0.f, -R, -H},
		};

		for (FVector& X : P)
		{
			Out.Add(Q.RotateVector(X) + S.Center);
		}
	}

	/** Generates points distributed evenly on a sphere using the Fibonacci Lattice algorithm. */
	FORCEINLINE void FibonacciSphere(int32 NumSamples, float Radius, const FTransform& Transform, TArray<FVector>& Out)
	{
		// Safety check: Ensure we have a valid number of samples to avoid division by zero
		if (NumSamples <= 0 || Radius <= 0.0f)
		{
			Out.Reset();
			return;
		}

		Out.Reset(NumSamples);

		const float InverseNumSamples = 1.0f / (float)NumSamples;

		for (int32 i = 0; i < NumSamples; ++i)
		{
			// Calculate Unit Sphere coordinates
			const float Z = 1.0f - (i * 2.0f + 1.0f) * InverseNumSamples;
			const float RadiusAtZ = FMath::Sqrt(1.0f - Z * Z);
			const float Theta = UE_TWO_PI * UE_GOLDEN_RATIO * i;

			float SinTheta, CosTheta;
			FMath::SinCos(&SinTheta, &CosTheta, Theta);

			const FVector UnitPoint(CosTheta * RadiusAtZ, SinTheta * RadiusAtZ, Z);

			// Apply Radius and Transform
			// We multiply by Radius first (Local scaling), then apply the Transform.
			Out.Add(Transform.TransformPosition(UnitPoint * Radius));
		}
	}
}