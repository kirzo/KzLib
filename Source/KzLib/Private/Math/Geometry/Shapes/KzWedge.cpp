// Copyright 2026 kirzo

#include "Math/Geometry/Shapes/KzWedge.h"
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
	/** Segments of the arc, as many per radian as a full circle of the same radius would get. */
	int32 WedgeArcSegments(const FKzWedge& Wedge)
	{
		const int32 CircleSides = FMath::Clamp(FMath::TruncToInt32(Wedge.Radius / 4.0f), 16, 64);
		return FMath::Max(2, FMath::CeilToInt32(CircleSides * Wedge.HalfAngle / 180.0f));
	}

	/** Point of the wedge at an angle from +X, a distance from its axis and a height. */
	FVector WedgePoint(double Angle, double Distance, double Height)
	{
		return FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, Height);
	}

	/** Calls DrawLine for each line of the wedge's outline: both caps and the vertical edges. */
	template<typename TDrawLine>
	void ForEachWedgeLine(const FKzWedge& Wedge, const FVector& Origin, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ, TDrawLine&& DrawLine)
	{
		auto ToWorld = [&](const FVector& Local)
		{
			return Origin + AxisX * Local.X + AxisY * Local.Y + AxisZ * Local.Z;
		};

		const double Angle = FMath::DegreesToRadians(Wedge.HalfAngle);
		const int32 Segments = WedgeArcSegments(Wedge);

		for (const float Height : { -Wedge.HalfHeight, Wedge.HalfHeight })
		{
			for (int32 Segment = 0; Segment < Segments; ++Segment)
			{
				const double From = -Angle + 2.0 * Angle * Segment / Segments;
				const double To = -Angle + 2.0 * Angle * (Segment + 1) / Segments;
				DrawLine(ToWorld(WedgePoint(From, Wedge.Radius, Height)), ToWorld(WedgePoint(To, Wedge.Radius, Height)));
			}
			DrawLine(ToWorld(WedgePoint(0.0, 0.0, Height)), ToWorld(WedgePoint(Angle, Wedge.Radius, Height)));
			DrawLine(ToWorld(WedgePoint(0.0, 0.0, Height)), ToWorld(WedgePoint(-Angle, Wedge.Radius, Height)));
		}

		DrawLine(ToWorld(WedgePoint(0.0, 0.0, -Wedge.HalfHeight)), ToWorld(WedgePoint(0.0, 0.0, Wedge.HalfHeight)));
		for (const double EdgeAngle : { -Angle, 0.0, Angle })
		{
			DrawLine(ToWorld(WedgePoint(EdgeAngle, Wedge.Radius, -Wedge.HalfHeight)), ToWorld(WedgePoint(EdgeAngle, Wedge.Radius, Wedge.HalfHeight)));
		}
	}
}

FBox FKzWedge::GetBoundingBox(const FVector& Origin, const FQuat& Rotation) const
{
	return Kz::Geom::WedgeBounds(Origin, Rotation, Radius, HalfAngle, HalfHeight);
}

FVector FKzWedge::GetClosestPoint(const FVector& Origin, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::ClosestPointOnWedge(Origin, Rotation, Radius, HalfAngle, HalfHeight, Point);
}

bool FKzWedge::IntersectsPoint(const FVector& Origin, const FQuat& Rotation, const FVector& Point) const
{
	return Kz::Geom::WedgeIntersectsPoint(Origin, Rotation, Radius, HalfAngle, HalfHeight, Point);
}

void FKzWedge::DrawDebug(const UWorld* InWorld, FVector const& Origin, const FQuat& Rotation, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
{
	ForEachWedgeLine(*this, Origin, Rotation.GetAxisX(), Rotation.GetAxisY(), Rotation.GetAxisZ(), [&](const FVector& Start, const FVector& End)
	{
		DrawDebugLine(InWorld, Start, End, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	});
}

void FKzWedge::DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const
{
	ForEachWedgeLine(*this, LocalToWorld.GetOrigin(), LocalToWorld.GetUnitAxis(EAxis::X), LocalToWorld.GetUnitAxis(EAxis::Y), LocalToWorld.GetUnitAxis(EAxis::Z), [&](const FVector& Start, const FVector& End)
	{
		PDI->DrawLine(Start, End, Color, DepthPriority, Thickness);
	});
}

void FKzWedge::DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const
{
	DrawWire(PDI, LocalToWorld, Color, Thickness, SDPG_World);

	if (bDrawSolid)
	{
		const double Angle = FMath::DegreesToRadians(HalfAngle);
		const int32 Segments = WedgeArcSegments(*this);

		// Per cap, the vertex on the axis and then the arc from -HalfAngle to +HalfAngle.
		FDynamicMeshBuilder MeshBuilder(Collector.GetFeatureLevel());
		for (const float Height : { -HalfHeight, HalfHeight })
		{
			MeshBuilder.AddVertex(FDynamicMeshVertex(FVector3f(WedgePoint(0.0, 0.0, Height))));
			for (int32 Segment = 0; Segment <= Segments; ++Segment)
			{
				MeshBuilder.AddVertex(FDynamicMeshVertex(FVector3f(WedgePoint(-Angle + 2.0 * Angle * Segment / Segments, Radius, Height))));
			}
		}

		const int32 Bottom = 0;
		const int32 Top = Segments + 2;
		for (int32 Segment = 0; Segment < Segments; ++Segment)
		{
			MeshBuilder.AddTriangle(Bottom, Bottom + 1 + Segment, Bottom + 2 + Segment);
			MeshBuilder.AddTriangle(Top, Top + 2 + Segment, Top + 1 + Segment);
			MeshBuilder.AddTriangle(Bottom + 1 + Segment, Top + 1 + Segment, Top + 2 + Segment);
			MeshBuilder.AddTriangle(Bottom + 1 + Segment, Top + 2 + Segment, Bottom + 2 + Segment);
		}

		// The two flat sides, from the axis to each end of the arc.
		MeshBuilder.AddTriangle(Bottom, Top, Top + 1);
		MeshBuilder.AddTriangle(Bottom, Top + 1, Bottom + 1);
		MeshBuilder.AddTriangle(Bottom, Bottom + 1 + Segments, Top + 1 + Segments);
		MeshBuilder.AddTriangle(Bottom, Top + 1 + Segments, Top);

		const FLinearColor SolidColor = FLinearColor(Color.R, Color.G, Color.B, 0.2f);
		const FColoredMaterialRenderProxy& MaterialRenderProxy = Collector.AllocateOneFrameResource<FColoredMaterialRenderProxy>(GEngine->DebugMeshMaterial->GetRenderProxy(), SolidColor);
		MeshBuilder.GetMesh(LocalToWorld.GetMatrixWithoutScale(), &MaterialRenderProxy, SDPG_World, true, false, ViewIndex, Collector);
	}
}