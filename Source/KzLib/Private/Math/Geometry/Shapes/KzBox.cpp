// Copyright 2026 kirzo

#include "Math/Geometry/Shapes/KzBox.h"
#include "Math/Geometry/KzGeometry.h"
#include "Collision/KzRaycast.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "DrawDebugHelpers.h"
#include "PrimitiveDrawingUtils.h"
#include "Materials/MaterialRenderProxy.h"

FBox FKzBox::GetBoundingBox(const FVector& Center, const FQuat& Rotation) const
{
	return Kz::Geom::BoxBounds(Center, Rotation, HalfSize);
}

FVector FKzBox::GetClosestPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::ClosestPointOnBox(Center, Rotation, HalfSize, Point);
}

bool FKzBox::IntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::BoxIntersectsPoint(Center, Rotation, HalfSize, Point);
}

bool FKzBox::Raycast(FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const
{
	return Kz::Raycast::Box(OutHit, Center, Rotation, HalfSize, RayStart, RayDir, MaxDistance);
}

void FKzBox::DrawDebug(const UWorld* InWorld, FVector const& Center, const FQuat& Rotation, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	DrawDebugBox(InWorld, Center, HalfSize, Rotation, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}

void FKzBox::DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const
{
	const FVector Origin = LocalToWorld.GetOrigin();
	const FVector UnitX = LocalToWorld.GetUnitAxis(EAxis::X);
	const FVector UnitY = LocalToWorld.GetUnitAxis(EAxis::Y);
	const FVector UnitZ = LocalToWorld.GetUnitAxis(EAxis::Z);

	DrawOrientedWireBox(PDI, Origin, UnitX, UnitY, UnitZ, HalfSize, Color, SDPG_World, Thickness);

	if (bDrawSolid)
	{
		const FLinearColor SolidColor = FLinearColor(Color.R, Color.G, Color.B, 0.2f);
		FMaterialRenderProxy* const MaterialRenderProxy = new FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(), SolidColor);
		GetBoxMesh(LocalToWorld.GetMatrixWithoutScale(), HalfSize, MaterialRenderProxy, SDPG_World, ViewIndex, Collector);
	}
}