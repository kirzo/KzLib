// Copyright 2026 kirzo

#include "Math/Geometry/Shapes/KzCapsule.h"
#include "Math/Geometry/KzGeometry.h"
#include "Collision/KzRaycast.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "DrawDebugHelpers.h"
#include "PrimitiveDrawingUtils.h"
#include "Materials/MaterialRenderProxy.h"

FBox FKzCapsule::GetBoundingBox(const FVector& Center, const FQuat& Rotation) const
{
	return Kz::Geom::CapsuleBounds(Center, Rotation, Radius, HalfHeight);
}

FVector FKzCapsule::GetClosestPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::ClosestPointOnCapsule(Center, Rotation, Radius, HalfHeight, Point);
}

bool FKzCapsule::IntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::CapsuleIntersectsPoint(Center, Rotation, Radius, HalfHeight, Point);
}

bool FKzCapsule::Raycast(FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const
{
	return Kz::Raycast::Capsule(OutHit, Center, Rotation, Radius, HalfHeight, RayStart, RayDir, MaxDistance);
}

void FKzCapsule::DrawDebug(const UWorld* InWorld, FVector const& Center, const FQuat& Rotation, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	DrawDebugCapsule(InWorld, Center, HalfHeight, Radius, Rotation, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}

void FKzCapsule::DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const
{
	const FVector Origin = LocalToWorld.GetOrigin();
	const FVector UnitX = LocalToWorld.GetUnitAxis(EAxis::X);
	const FVector UnitY = LocalToWorld.GetUnitAxis(EAxis::Y);
	const FVector UnitZ = LocalToWorld.GetUnitAxis(EAxis::Z);

	const int32 CapsuleSides = FMath::Clamp<int32>(Radius / 4.f, 16, 64);
	DrawWireCapsule(PDI, Origin, UnitX, UnitY, UnitZ, Color, Radius, HalfHeight, CapsuleSides, SDPG_World, Thickness);

	if (bDrawSolid)
	{
		const FVector Bottom = Origin - UnitZ * HalfHeight;

		const FLinearColor SolidColor = FLinearColor(Color.R, Color.G, Color.B, 0.2f);
		FMaterialRenderProxy* const MaterialRenderProxy = new FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(), SolidColor);
		GetCapsuleMesh(Bottom, UnitX, UnitY, UnitZ, SolidColor, Radius, HalfHeight, CapsuleSides, MaterialRenderProxy, SDPG_World, false, ViewIndex, Collector);
	}
}