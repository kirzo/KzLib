// Copyright 2025 kirzo

#include "Kismet/KzSystemLibrary.h"
#include "Collision/KzHitResult.h"
#include "Math/KzRandom.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

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

void UKzSystemLibrary::CopyObjectProperties(UObject* Source, UObject* Target, bool bCopyTransients)
{
	if (!Source || !Target || Source == Target)
	{
		return;
	}

	for (TFieldIterator<FProperty> It(Source->GetClass()); It; ++It)
	{
		FProperty* SourceProp = *It;

		// Skip Deprecated properties always
		if (SourceProp->HasAnyPropertyFlags(CPF_Deprecated))
		{
			continue;
		}

		// Handle Transient properties
		// If we are NOT copying transients, and this property IS transient, skip it.
		if (!bCopyTransients && SourceProp->HasAnyPropertyFlags(CPF_Transient))
		{
			continue;
		}

		// Find matching property in Target
		FProperty* TargetProp = Target->GetClass()->FindPropertyByName(SourceProp->GetFName());

		if (!TargetProp)
		{
			continue;
		}

		// Verify types match
		if (!SourceProp->SameType(TargetProp))
		{
			continue;
		}

		// Copy the value
		const void* SrcValuePtr = SourceProp->ContainerPtrToValuePtr<void>(Source);
		void* TgtValuePtr = TargetProp->ContainerPtrToValuePtr<void>(Target);

		SourceProp->CopyCompleteValue(TgtValuePtr, SrcValuePtr);
	}
}

UActorComponent* UKzSystemLibrary::FindComponentInActorOrController(AActor* Target, TSubclassOf<UActorComponent> ComponentClass)
{
	if (!Target || !ComponentClass)
	{
		return nullptr;
	}

	// Try Actor
	UActorComponent* FoundComp = Target->FindComponentByClass(ComponentClass);
	if (FoundComp)
	{
		return FoundComp;
	}

	// Try Controller (if Pawn)
	if (APawn* Pawn = Cast<APawn>(Target))
	{
		if (AController* Controller = Pawn->GetController())
		{
			return Controller->FindComponentByClass(ComponentClass);
		}
	}

	return nullptr;
}