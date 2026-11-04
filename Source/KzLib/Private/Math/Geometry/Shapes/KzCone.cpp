// Copyright 2026 kirzo

#include "Math/Geometry/Shapes/KzCone.h"
#include "Math/Geometry/KzGeometry.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "DrawDebugHelpers.h"
#include "PrimitiveDrawingUtils.h"
#include "Materials/MaterialRenderProxy.h"
#include "DynamicMeshBuilder.h"
#include "MeshElementCollector.h"

namespace
{
	constexpr int32 ConeCapSegments = 8;
	constexpr int32 ConeMeridians = 4;

	int32 ConeSides(const FKzCone& Cone)
	{
		return FMath::Clamp(FMath::TruncToInt32(Kz::Geom::ConeRim(Cone.Length, Cone.HalfAngle, Cone.TipRadius, Cone.bSphericalCap).Y / 4.0), 16, 64);
	}

	/** Calls DrawLine for each line of the cone's outline: a circle at the tip and at the rim, and the meridians. */
	template<typename TDrawLine>
	void ForEachConeLine(const FKzCone& Cone, const FVector& Origin, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ, TDrawLine&& DrawLine)
	{
		auto ToWorld = [&](const FVector2D& Point, double Angle)
		{
			return Origin + AxisX * Point.X + (AxisY * FMath::Cos(Angle) + AxisZ * FMath::Sin(Angle)) * Point.Y;
		};

		const int32 Sides = ConeSides(Cone);
		auto DrawRing = [&](const FVector2D& Point)
		{
			for (int32 Side = 0; Side < Sides; ++Side)
			{
				DrawLine(ToWorld(Point, UE_TWO_PI * Side / Sides), ToWorld(Point, UE_TWO_PI * (Side + 1) / Sides));
			}
		};

		if (Cone.TipRadius > 0.0f)
		{
			DrawRing(FVector2D(0.0, Cone.TipRadius));
		}
		DrawRing(Kz::Geom::ConeRim(Cone.Length, Cone.HalfAngle, Cone.TipRadius, Cone.bSphericalCap));

		TArray<FVector2D> Outline;
		Kz::Geom::ConeOutline(Cone.Length, Cone.HalfAngle, Cone.TipRadius, Cone.bSphericalCap, ConeCapSegments, Outline);

		// A truncated cone's meridians start at the tip's rim, not at its center.
		const int32 First = Cone.TipRadius > 0.0f ? 1 : 0;
		for (int32 Meridian = 0; Meridian < ConeMeridians; ++Meridian)
		{
			const double Angle = UE_TWO_PI * Meridian / ConeMeridians;
			for (int32 Index = First; Index + 1 < Outline.Num(); ++Index)
			{
				DrawLine(ToWorld(Outline[Index], Angle), ToWorld(Outline[Index + 1], Angle));
			}
		}
	}
}

FBox FKzCone::GetBoundingBox(const FVector& Origin, const FQuat& Rotation) const
{
	return Kz::Geom::ConeBounds(Origin, Rotation, Length, HalfAngle, TipRadius, bSphericalCap);
}

FVector FKzCone::GetClosestPoint(const FVector& Origin, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::ClosestPointOnCone(Origin, Rotation, Length, HalfAngle, TipRadius, bSphericalCap, Point);
}

bool FKzCone::IntersectsPoint(const FVector& Origin, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::ConeIntersectsPoint(Origin, Rotation, Length, HalfAngle, TipRadius, bSphericalCap, Point);
}

FCollisionShape FKzCone::ToCollisionShape(float Inflation) const
{
	// A sphere around the tip: the farthest points from it are the rim, the end of the axis or the tip's rim.
	const double Rim = Kz::Geom::ConeRim(Length, HalfAngle, TipRadius, bSphericalCap).Size();
	return FCollisionShape::MakeSphere(static_cast<float>(FMath::Max3(double(Length), Rim, double(TipRadius))) + Inflation);
}

void FKzCone::DrawDebug(const UWorld* InWorld, FVector const& Origin, const FQuat& Rotation, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	ForEachConeLine(*this, Origin, Rotation.GetAxisX(), Rotation.GetAxisY(), Rotation.GetAxisZ(), [&](const FVector& Start, const FVector& End)
	{
		DrawDebugLine(InWorld, Start, End, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	});
}

void FKzCone::DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const
{
	ForEachConeLine(*this, LocalToWorld.GetOrigin(), LocalToWorld.GetUnitAxis(EAxis::X), LocalToWorld.GetUnitAxis(EAxis::Y), LocalToWorld.GetUnitAxis(EAxis::Z), [&](const FVector& Start, const FVector& End)
	{
		PDI->DrawLine(Start, End, Color, DepthPriority, Thickness);
	});
}

void FKzCone::DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const
{
	DrawWire(PDI, LocalToWorld, Color, Thickness, SDPG_World);

	if (bDrawSolid)
	{
		TArray<FVector2D> Outline;
		Kz::Geom::ConeOutline(Length, HalfAngle, TipRadius, bSphericalCap, ConeCapSegments, Outline);
		const int32 Sides = ConeSides(*this);

		// The outline turned around the axis, one ring of vertices per outline point.
		FDynamicMeshBuilder MeshBuilder(Collector.GetFeatureLevel());
		for (const FVector2D& Point : Outline)
		{
			for (int32 Side = 0; Side < Sides; ++Side)
			{
				const double Angle = UE_TWO_PI * Side / Sides;
				MeshBuilder.AddVertex(FDynamicMeshVertex(FVector3f(FVector(Point.X, Point.Y * FMath::Cos(Angle), Point.Y * FMath::Sin(Angle)))));
			}
		}

		for (int32 Ring = 0; Ring + 1 < Outline.Num(); ++Ring)
		{
			for (int32 Side = 0; Side < Sides; ++Side)
			{
				const int32 Next = (Side + 1) % Sides;
				const int32 A = Ring * Sides + Side;
				const int32 B = Ring * Sides + Next;
				const int32 C = (Ring + 1) * Sides + Next;
				const int32 D = (Ring + 1) * Sides + Side;
				MeshBuilder.AddTriangle(A, B, C);
				MeshBuilder.AddTriangle(A, C, D);
			}
		}

		const FLinearColor SolidColor = FLinearColor(Color.R, Color.G, Color.B, 0.2f);
		const FColoredMaterialRenderProxy& MaterialRenderProxy = Collector.AllocateOneFrameResource<FColoredMaterialRenderProxy>(GEngine->DebugMeshMaterial->GetRenderProxy(), SolidColor);
		MeshBuilder.GetMesh(LocalToWorld.GetMatrixWithoutScale(), &MaterialRenderProxy, SDPG_World, true, false, ViewIndex, Collector);
	}
}