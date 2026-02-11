// Copyright 2026 kirzo

#include "ComponentVisualizers/KzActorGroupVisualizer.h"
#include "Actors/KzActorGroup.h"
#include "EngineUtils.h"

static const FLinearColor KzGroupColor = FLinearColor::Yellow;

void GetBoundingVectorsForGroup(AKzActorGroup* ActorGroup, FVector& OutVectorMin, FVector& OutVectorMax)
{
	// Initialize with opposite extremes
	OutVectorMin = FVector(UE_BIG_NUMBER);
	OutVectorMax = FVector(-UE_BIG_NUMBER);

	bool bFoundAny = false;

	for (const AActor* Actor : ActorGroup->Actors)
	{
		if (IsValid(Actor))
		{
			const FBox ActorBox = Actor->GetComponentsBoundingBox(false);

			// Update MinVector
			OutVectorMin.X = FMath::Min<FVector::FReal>(ActorBox.Min.X, OutVectorMin.X);
			OutVectorMin.Y = FMath::Min<FVector::FReal>(ActorBox.Min.Y, OutVectorMin.Y);
			OutVectorMin.Z = FMath::Min<FVector::FReal>(ActorBox.Min.Z, OutVectorMin.Z);

			// Update MaxVector
			OutVectorMax.X = FMath::Max<FVector::FReal>(ActorBox.Max.X, OutVectorMax.X);
			OutVectorMax.Y = FMath::Max<FVector::FReal>(ActorBox.Max.Y, OutVectorMax.Y);
			OutVectorMax.Z = FMath::Max<FVector::FReal>(ActorBox.Max.Z, OutVectorMax.Z);

			bFoundAny = true;
		}
	}

	// Fallback if no actors are present to avoid giant box
	if (!bFoundAny)
	{
		OutVectorMin = ActorGroup->GetActorLocation();
		OutVectorMax = ActorGroup->GetActorLocation();
	}
}

void FKzActorGroupVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const AKzActorGroup* ActorGroup = Component ? Cast<AKzActorGroup>(Component->GetOwner()) : nullptr;
	if (!ActorGroup)
	{
		return;
	}

	const FVector StartLoc = ActorGroup->GetActorLocation();

	// 1. Draw connection lines from the Group Actor to each member
	for (const AActor* Actor : ActorGroup->Actors)
	{
		if (IsValid(Actor))
		{
			PDI->DrawLine(StartLoc, Actor->GetActorLocation(), FColor::Green, SDPG_Foreground);
		}
	}

	// 2. Draw brackets around the entire collection
	if (ActorGroup->Actors.Num() > 0)
	{
		FVector MinVector;
		FVector MaxVector;
		GetBoundingVectorsForGroup(const_cast<AKzActorGroup*>(ActorGroup), MinVector, MaxVector);

		// Pad the brackets slightly
		const FVector BracketOffset = FVector(FVector::Dist(MinVector, MaxVector) * 0.1f); // 10% padding
		MinVector -= BracketOffset;
		MaxVector += BracketOffset;

		TArray<FVector> BracketCorners;
		BracketCorners.Reserve(8);

		// Bottom Corners
		BracketCorners.Add(FVector(MinVector.X, MinVector.Y, MinVector.Z));
		BracketCorners.Add(FVector(MinVector.X, MaxVector.Y, MinVector.Z));
		BracketCorners.Add(FVector(MaxVector.X, MaxVector.Y, MinVector.Z));
		BracketCorners.Add(FVector(MaxVector.X, MinVector.Y, MinVector.Z));

		// Top Corners
		BracketCorners.Add(FVector(MinVector.X, MinVector.Y, MaxVector.Z));
		BracketCorners.Add(FVector(MinVector.X, MaxVector.Y, MaxVector.Z));
		BracketCorners.Add(FVector(MaxVector.X, MaxVector.Y, MaxVector.Z));
		BracketCorners.Add(FVector(MaxVector.X, MinVector.Y, MaxVector.Z));

		// Draw small corner markers at each vertex of the bounding box
		for (const FVector& Corner : BracketCorners)
		{
			const int32 DirX = (Corner.X == MaxVector.X) ? -1 : 1;
			const int32 DirY = (Corner.Y == MaxVector.Y) ? -1 : 1;
			const int32 DirZ = (Corner.Z == MaxVector.Z) ? -1 : 1;

			const float CornerLength = BracketOffset.X;

			PDI->DrawLine(Corner, FVector(Corner.X + (CornerLength * DirX), Corner.Y, Corner.Z), KzGroupColor, SDPG_Foreground);
			PDI->DrawLine(Corner, FVector(Corner.X, Corner.Y + (CornerLength * DirY), Corner.Z), KzGroupColor, SDPG_Foreground);
			PDI->DrawLine(Corner, FVector(Corner.X, Corner.Y, Corner.Z + (CornerLength * DirZ)), KzGroupColor, SDPG_Foreground);
		}
	}
}