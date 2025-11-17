// Copyright 2025 kirzo

#include "Kismet/KzSystemLibrary.h"
#include "Collision/KzHitResult.h"
#include "Math/KzRandom.h"

void UKzSystemLibrary::BreakHitResult(const FKzHitResult& Hit, bool& bBlockingHit, bool& bInitialOverlap, float& Time, float& Distance, FVector& Location, FVector& Normal, FVector& TraceStart, FVector& TraceEnd)
{
	bBlockingHit = Hit.bBlockingHit;
	bInitialOverlap = Hit.bStartPenetrating;
	Time = Hit.Time;
	Distance = Hit.Distance;
	Location = Hit.Location;
	Normal = Hit.Normal;
	TraceStart = Hit.TraceStart;
	TraceEnd = Hit.TraceEnd;
}

float UKzSystemLibrary::GaussianFloat()
{
	return Kz::Random::Gaussian();
}

float UKzSystemLibrary::GaussianFloatFromStream(UPARAM(ref)FRandomStream& Stream)
{
	return Kz::Random::Gaussian(Stream);
}

FVector UKzSystemLibrary::GaussianVector()
{
	return Kz::Random::GaussianVector();
}

FVector UKzSystemLibrary::GaussianVectorFromStream(UPARAM(ref)FRandomStream& Stream)
{
	return Kz::Random::GaussianVector(Stream);
}