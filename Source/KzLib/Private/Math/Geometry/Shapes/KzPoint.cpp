// Copyright 2026 kirzo

#include "Math/Geometry/Shapes/KzPoint.h"
#include "DrawDebugHelpers.h"
#include "PrimitiveDrawingUtils.h"

namespace
{
	/** Half the length, in cm, of each arm of the cross a point is drawn as. */
	constexpr float PointDrawSize = 10.0f;
}

void FKzPoint::DrawDebug(const UWorld* InWorld, FVector const& Position, const FQuat& Rotation, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	for (const FVector& Axis : { Rotation.GetAxisX(), Rotation.GetAxisY(), Rotation.GetAxisZ() })
	{
		DrawDebugLine(InWorld, Position - Axis * PointDrawSize, Position + Axis * PointDrawSize, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	}
}

void FKzPoint::DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const
{
	const FVector Origin = LocalToWorld.GetOrigin();
	for (const EAxis::Type Axis : { EAxis::X, EAxis::Y, EAxis::Z })
	{
		const FVector Arm = LocalToWorld.GetUnitAxis(Axis) * PointDrawSize;
		PDI->DrawLine(Origin - Arm, Origin + Arm, Color, DepthPriority, Thickness);
	}
}

void FKzPoint::DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const
{
	// No solid to fill: the cross is all of it
	DrawWire(PDI, LocalToWorld, Color, Thickness, SDPG_World);
}