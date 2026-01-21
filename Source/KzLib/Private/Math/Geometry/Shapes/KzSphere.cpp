// Copyright 2026 kirzo

#include "Math/Geometry/Shapes/KzSphere.h"
#include "Math/Geometry/KzGeometry.h"
#include "Collision/KzRaycast.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "DrawDebugHelpers.h"
#include "PrimitiveDrawingUtils.h"
#include "Materials/MaterialRenderProxy.h"

FBox FKzSphere::GetBoundingBox(const FVector& Center, const FQuat& Rotation) const
{
	return Kz::Geom::SphereBounds(Center, Radius);
}

FVector FKzSphere::GetClosestPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::ClosestPointOnSphere(Center, Radius, Point);
}

bool FKzSphere::IntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::SphereIntersectsPoint(Center, Radius, Point);
}

bool FKzSphere::Raycast(struct FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const
{
	return Kz::Raycast::Sphere(OutHit, Center, Radius, RayStart, RayDir, MaxDistance);
}

void FKzSphere::DrawDebug(const UWorld* InWorld, FVector const& Center, const FQuat& Rotation, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	DrawDebugSphere(InWorld, Center, Radius, 12, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}

void FKzSphere::DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const
{
	const FVector Origin = LocalToWorld.GetOrigin();
	const FVector UnitX = LocalToWorld.GetUnitAxis(EAxis::X);
	const FVector UnitY = LocalToWorld.GetUnitAxis(EAxis::Y);
	const FVector UnitZ = LocalToWorld.GetUnitAxis(EAxis::Z);

	const int32 SphereSides = FMath::Clamp<int32>(Radius / 4.f, 16, 64);
	DrawCircle(PDI, Origin, UnitX, UnitY, Color, Radius, SphereSides, SDPG_World, Thickness);
	DrawCircle(PDI, Origin, UnitX, UnitZ, Color, Radius, SphereSides, SDPG_World, Thickness);
	DrawCircle(PDI, Origin, UnitY, UnitZ, Color, Radius, SphereSides, SDPG_World, Thickness);

	if (bDrawSolid)
	{
		const FLinearColor SolidColor = FLinearColor(Color.R, Color.G, Color.B, 0.2f);
		FMaterialRenderProxy* const MaterialRenderProxy = new FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(), SolidColor);
		GetOrientedHalfSphereMesh(Origin, LocalToWorld.Rotator(), FVector(Radius), SphereSides, SphereSides, 0, UE_PI, MaterialRenderProxy, SDPG_World, false, ViewIndex, Collector);
	}
}