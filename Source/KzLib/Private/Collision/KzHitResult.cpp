// Copyright 2026 kirzo

#include "Collision/KzHitResult.h"
#include "Engine/HitResult.h"

bool FKzHitResult::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	// pack bitfield with flags
	uint8 Flags = (bBlockingHit << 0) | (bStartPenetrating << 1);
	Ar.SerializeBits(&Flags, 8);
	bBlockingHit = (Flags & (1 << 0)) ? 1 : 0;
	bStartPenetrating = (Flags & (1 << 1)) ? 1 : 0;

	Ar << Time;

	bOutSuccess = true;

	bool bOutSuccessLocal = true;

	Location.NetSerialize(Ar, Map, bOutSuccessLocal);
	bOutSuccess &= bOutSuccessLocal;
	Normal.NetSerialize(Ar, Map, bOutSuccessLocal);
	bOutSuccess &= bOutSuccessLocal;

	TraceStart.NetSerialize(Ar, Map, bOutSuccessLocal);
	bOutSuccess &= bOutSuccessLocal;
	TraceEnd.NetSerialize(Ar, Map, bOutSuccessLocal);
	bOutSuccess &= bOutSuccessLocal;

	if (Ar.IsLoading() && bOutSuccess)
	{
		Distance = (Location - TraceStart).Size();
	}

	return true;
}

FString FKzHitResult::ToString() const
{
	return FString::Printf(TEXT("bBlockingHit:%s bStartPenetrating:%s Time:%f Location:%s Normal:%s TraceStart:%s TraceEnd:%s"),
		bBlockingHit == true ? TEXT("True") : TEXT("False"),
		bStartPenetrating == true ? TEXT("True") : TEXT("False"),
		Time,
		*Location.ToString(),
		*Normal.ToString(),
		*TraceStart.ToString(),
		*TraceEnd.ToString());
}

FHitResult FKzHitResult::ToHitResult() const
{
	FHitResult Hit;
	Hit.bBlockingHit = bBlockingHit;
	Hit.bStartPenetrating = bStartPenetrating;
	Hit.Time = Time;
	Hit.Distance = Distance;
	Hit.Location = Location;
	Hit.ImpactPoint = Location;
	Hit.Normal = Normal;
	Hit.ImpactNormal = Normal;
	Hit.TraceStart = TraceStart;
	Hit.TraceEnd = TraceEnd;
	return Hit;
}