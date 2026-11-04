// Copyright 2026 kirzo

#include "Math/Geometry/Shapes/KzCapsule.h"
#include "Math/Geometry/KzGeometry.h"
#include "Collision/KzRaycast.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "DrawDebugHelpers.h"
#include "PrimitiveDrawingUtils.h"
#include "Materials/MaterialRenderProxy.h"
#include "MeshElementCollector.h"

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

void FKzCapsule::DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const
{
	const int32 CapsuleSides = FMath::Clamp<int32>(Radius / 4.f, 16, 64);
	DrawWireCapsule(PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetUnitAxis(EAxis::X), LocalToWorld.GetUnitAxis(EAxis::Y), LocalToWorld.GetUnitAxis(EAxis::Z), Color, Radius, HalfHeight, CapsuleSides, DepthPriority, Thickness);
}

void FKzCapsule::DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const
{
	DrawWire(PDI, LocalToWorld, Color, Thickness, SDPG_World);

	if (bDrawSolid)
	{
		const FVector UnitX = LocalToWorld.GetUnitAxis(EAxis::X);
		const FVector UnitY = LocalToWorld.GetUnitAxis(EAxis::Y);
		const FVector UnitZ = LocalToWorld.GetUnitAxis(EAxis::Z);
		const FVector Bottom = LocalToWorld.GetOrigin() - UnitZ * HalfHeight;
		const int32 CapsuleSides = FMath::Clamp<int32>(Radius / 4.f, 16, 64);

		const FLinearColor SolidColor = FLinearColor(Color.R, Color.G, Color.B, 0.2f);
		const FColoredMaterialRenderProxy& MaterialRenderProxy = Collector.AllocateOneFrameResource<FColoredMaterialRenderProxy>(GEngine->DebugMeshMaterial->GetRenderProxy(), SolidColor);
		GetCapsuleMesh(Bottom, UnitX, UnitY, UnitZ, SolidColor, Radius, HalfHeight, CapsuleSides, &MaterialRenderProxy, SDPG_World, false, ViewIndex, Collector);
	}
}