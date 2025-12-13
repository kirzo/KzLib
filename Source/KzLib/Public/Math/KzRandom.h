// Copyright 2025 kirzo

#pragma once

namespace Kz::Random
{
	namespace Internal
	{
		/**
		 * Generic Box–Muller implementation.
		 */
		FORCEINLINE float GaussianBoxMuller(float Uniform1, float Uniform2)
		{
			Uniform1 = FMath::Max(Uniform1, UE_KINDA_SMALL_NUMBER); // avoid log(0)

			const float Radius = FMath::Sqrt(-2.0f * FMath::Loge(Uniform1));
			const float Theta = UE_TWO_PI * Uniform2;

			return Radius * FMath::Cos(Theta);
		}
	}

	/** Returns a normally-distributed random number N(0, 1). */
	FORCEINLINE float Gaussian()
	{
		return Internal::GaussianBoxMuller(FMath::FRand(), FMath::FRand());
	}

	/** Returns a normally-distributed random number N(0, 1). */
	FORCEINLINE float Gaussian(FRandomStream& Stream)
	{
		return Internal::GaussianBoxMuller(Stream.GetFraction(), Stream.GetFraction());
	}

	/** Returns a Gaussian distributed number N(Mean, StdDev). */
	FORCEINLINE float GaussianRange(float Mean, float StdDev)
	{
		return Mean + Gaussian() * StdDev;
	}

	/** Returns a Gaussian distributed number N(Mean, StdDev) using a stream. */
	FORCEINLINE float GaussianRange(FRandomStream& Stream, float Mean, float StdDev)
	{
		return Mean + Gaussian(Stream) * StdDev;
	}

	/** Returns a 3D vector where each component ~ N(0, 1). */
	FORCEINLINE FVector GaussianVector()
	{
		return FVector(Gaussian(), Gaussian(), Gaussian());
	}

	/** Returns a 3D vector where each component ~ N(0, 1) using a stream. */
	FORCEINLINE FVector GaussianVector(FRandomStream& Stream)
	{
		return FVector(Gaussian(Stream), Gaussian(Stream), Gaussian(Stream));
	}

	/** Returns a vector with each component ~ N(Mean, StdDev). */
	FORCEINLINE FVector GaussianVectorRange(const FVector& Mean, const FVector& StdDev)
	{
		return Mean + GaussianVector() * StdDev;
	}

	/** Returns a vector with each component ~ N(Mean, StdDev) using a stream. */
	FORCEINLINE FVector GaussianVectorRange(FRandomStream& Stream, const FVector& Mean, const FVector& StdDev)
	{
		return Mean + GaussianVector(Stream) * StdDev;
	}
}