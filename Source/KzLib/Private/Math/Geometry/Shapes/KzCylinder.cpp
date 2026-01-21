// Copyright 2026 kirzo

#include "Math/Geometry/Shapes/KzCylinder.h"
#include "Math/Geometry/KzGeometry.h"
#include "Collision/KzRaycast.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "DrawDebugHelpers.h"
#include "PrimitiveDrawingUtils.h"
#include "Materials/MaterialRenderProxy.h"

FBox FKzCylinder::GetBoundingBox(const FVector& Center, const FQuat& Rotation) const
{
	return Kz::Geom::CylinderBounds(Center, Rotation, Radius, HalfHeight);
}

FVector FKzCylinder::GetClosestPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::ClosestPointOnCylinder(Center, Rotation, Radius, HalfHeight, Point);
}

bool FKzCylinder::IntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::CylinderIntersectsPoint(Center, Rotation, Radius, HalfHeight, Point);
}

bool FKzCylinder::Raycast(FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const
{
	return Kz::Raycast::Cylinder(OutHit, Center, Rotation, Radius, HalfHeight, RayStart, RayDir, MaxDistance);
}

void FKzCylinder::DrawDebug(const UWorld* InWorld, FVector const& Center, const FQuat& Rotation, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	const FVector UpVector = Rotation.GetUpVector();
	DrawDebugCylinder(InWorld, Center - UpVector * HalfHeight, Center + UpVector * HalfHeight, Radius, 12, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}

void FKzCylinder::DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const
{
	const FVector Origin = LocalToWorld.GetOrigin();
	const FVector UnitX = LocalToWorld.GetUnitAxis(EAxis::X);
	const FVector UnitY = LocalToWorld.GetUnitAxis(EAxis::Y);
	const FVector UnitZ = LocalToWorld.GetUnitAxis(EAxis::Z);

	const int32 CylinderSides = FMath::Clamp<int32>(Radius / 50, 16, 64);
	DrawWireCylinder(PDI, Origin, UnitX, UnitY, UnitZ, Color, Radius, HalfHeight, CylinderSides, SDPG_World, Thickness);

	if (bDrawSolid)
	{
		const FLinearColor SolidColor = FLinearColor(Color.R, Color.G, Color.B, 0.2f);

		FMaterialRenderProxy* const MaterialRenderProxy = new FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(), SolidColor);
		GetCylinderMesh(FMatrix::Identity, Origin, UnitX, UnitY, UnitZ, Radius, HalfHeight, CylinderSides, MaterialRenderProxy, SDPG_World, ViewIndex, Collector);
	}
}