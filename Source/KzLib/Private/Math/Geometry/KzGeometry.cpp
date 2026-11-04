// Copyright 2026 kirzo

#include "Math/Geometry/KzGeometry.h"
#include "Math/Box.h"
#include "Math/Vector.h"
#include "Math/Quat.h"
#include "Math/Vector2D.h"

namespace
{
	/** Exact world bounds of a convex shape from its local support function. */
	template<typename TSupport>
	FBox BoundsFromSupport(const FVector& Origin, const FQuat& Rotation, TSupport&& Support)
	{
		FVector Min, Max;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			FVector WorldAxis = FVector::ZeroVector;
			WorldAxis[Axis] = 1.0;
			const FVector LocalAxis = Rotation.UnrotateVector(WorldAxis);
			Max[Axis] = Rotation.RotateVector(Support(LocalAxis))[Axis];
			Min[Axis] = Rotation.RotateVector(Support(-LocalAxis))[Axis];
		}
		return FBox(Origin + Min, Origin + Max);
	}

	/** Puts a point of a shape of revolution around X back in 3D, on the side of the axis that Radial points to. */
	FVector FromAxialPlane(const FVector2D& Point, const FVector& Radial)
	{
		const double RadialSize = Radial.Size();
		return FVector(Point.X, 0.0, 0.0) + (RadialSize > UE_KINDA_SMALL_NUMBER ? Radial * (Point.Y / RadialSize) : FVector::ZeroVector);
	}

	/** A cone in its axial half plane: X along the axis, Y away from it. */
	struct FConeSection
	{
		double Length;
		double TipRadius;
		double Angle;
		double Tan;
		bool bSphericalCap;

		/** The spherical cap is centered on the apex the side would reach behind the tip. */
		double CapCenter = 0.0;
		double CapRadius = 0.0;

		/** Where the side meets the cap. */
		FVector2D Rim;

		FConeSection(float InLength, float HalfAngle, float InTipRadius, bool bInSphericalCap)
			: Length(FMath::Max(0.0f, InLength))
			, TipRadius(FMath::Max(0.0f, InTipRadius))
			, Angle(FMath::DegreesToRadians(FMath::Clamp(HalfAngle, 0.0f, Kz::Geom::MaxConeHalfAngle)))
		{
			Tan = FMath::Tan(Angle);

			// With no angle there is no apex to center a sphere on, so the cap stays flat.
			bSphericalCap = bInSphericalCap && Tan > UE_KINDA_SMALL_NUMBER;
			if (bSphericalCap)
			{
				CapCenter = -TipRadius / Tan;
				CapRadius = Length - CapCenter;
				Rim = OnCap(Angle);
			}
			else
			{
				Rim = FVector2D(Length, TipRadius + Length * Tan);
			}
		}

		/** Point of the spherical cap at an angle from the axis, clamped to the cap. */
		FVector2D OnCap(double ArcAngle) const
		{
			const double Clamped = FMath::Clamp(ArcAngle, 0.0, Angle);
			return FVector2D(CapCenter, 0.0) + CapRadius * FVector2D(FMath::Cos(Clamped), FMath::Sin(Clamped));
		}

		bool Contains(const FVector2D& P) const
		{
			if (P.X < 0.0 || P.Y > TipRadius + P.X * Tan)
			{
				return false;
			}
			return bSphericalCap ? FVector2D::DistSquared(P, FVector2D(CapCenter, 0.0)) <= FMath::Square(CapRadius) : P.X <= Length;
		}

		/** Direction.Y must not be negative, as the radial part of a 3D direction. */
		FVector2D Support(const FVector2D& Direction) const
		{
			const FVector2D TipRim(0.0, TipRadius);
			const FVector2D Far = bSphericalCap ? OnCap(FMath::Atan2(Direction.Y, Direction.X)) : Rim;
			return (TipRim | Direction) > (Far | Direction) ? TipRim : Far;
		}

		/** P.Y must not be negative. The axis is not a boundary: a point off it is always closer to the outline. */
		FVector2D Closest(const FVector2D& P) const
		{
			if (Contains(P))
			{
				return P;
			}

			const FVector2D TipRim(0.0, TipRadius);
			FVector2D Best = FMath::ClosestPointOnSegment2D(P, FVector2D::ZeroVector, TipRim);
			auto Consider = [&P, &Best](const FVector2D& Candidate)
			{
				if (FVector2D::DistSquared(Candidate, P) < FVector2D::DistSquared(Best, P))
				{
					Best = Candidate;
				}
			};

			Consider(FMath::ClosestPointOnSegment2D(P, TipRim, Rim));
			Consider(bSphericalCap ? OnCap(FMath::Atan2(P.Y, P.X - CapCenter)) : FMath::ClosestPointOnSegment2D(P, Rim, FVector2D(Length, 0.0)));
			return Best;
		}

		FVector Support(const FVector& Direction) const
		{
			const FVector Radial(0.0, Direction.Y, Direction.Z);
			return FromAxialPlane(Support(FVector2D(Direction.X, Radial.Size())), Radial);
		}
	};

	/** A wedge in its horizontal plane: a circular sector opening along +X. */
	struct FWedgeSection
	{
		double Radius;
		double Angle;
		double Cos;

		/** End of the +Y side on the arc; the -Y side is its mirror. */
		FVector2D Edge;

		FWedgeSection(float InRadius, float HalfAngle)
			: Radius(FMath::Max(0.0f, InRadius))
			, Angle(FMath::DegreesToRadians(FMath::Clamp(HalfAngle, 0.0f, 90.0f)))
		{
			Cos = FMath::Cos(Angle);
			Edge = Radius * FVector2D(Cos, FMath::Sin(Angle));
		}

		bool Contains(const FVector2D& P) const
		{
			return P.SizeSquared() <= FMath::Square(Radius) && P.X >= P.Size() * Cos;
		}

		bool FacesArc(const FVector2D& Direction) const
		{
			return FMath::Abs(FMath::Atan2(Direction.Y, Direction.X)) <= Angle;
		}

		FVector2D Support(const FVector2D& Direction) const
		{
			if (FacesArc(Direction))
			{
				return Direction.GetSafeNormal() * Radius;
			}

			const FVector2D Mirror(Edge.X, -Edge.Y);
			const FVector2D Best = (Edge | Direction) >= (Mirror | Direction) ? Edge : Mirror;
			return (Best | Direction) > 0.0 ? Best : FVector2D::ZeroVector;
		}

		FVector2D Closest(const FVector2D& P) const
		{
			if (Contains(P))
			{
				return P;
			}

			FVector2D Best = FMath::ClosestPointOnSegment2D(P, FVector2D::ZeroVector, Edge);
			auto Consider = [&P, &Best](const FVector2D& Candidate)
			{
				if (FVector2D::DistSquared(Candidate, P) < FVector2D::DistSquared(Best, P))
				{
					Best = Candidate;
				}
			};

			Consider(FMath::ClosestPointOnSegment2D(P, FVector2D::ZeroVector, FVector2D(Edge.X, -Edge.Y)));
			if (FacesArc(P))
			{
				Consider(P.GetSafeNormal() * Radius);
			}
			return Best;
		}
	};
}

namespace Kz::Geom
{
	float DistanceToLine(const FVector& A, const FVector& B, const FVector& P)
	{
		if (A.Equals(B))
		{
			return FVector::Distance(A, P);
		}

		const FVector PA = P - A;
		const FVector N = (B - A).GetSafeNormal();
		return (PA - (PA | N) * N).Size();
	}

	TArray<FVector> SimplifyPolygon(TArray<FVector> Polygon, float AngleThreshold)
	{
		int32 Index = 0;
		float Angle = 0.0f;

		while (Index < Polygon.Num() - 2)
		{
			const FVector P1 = Polygon[Index];
			const FVector P2 = Polygon[Index + 1];
			const FVector P3 = Polygon[Index + 2];

			const FVector U = (P2 - P1).GetSafeNormal();
			const FVector V = (P3 - P1).GetSafeNormal();

			const FVector W = U ^ V;

			Angle += FMath::Sign((W ^ U) | V) * FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(U | V, -1.0f, 1.0f)));

			if (FMath::Abs(Angle) < AngleThreshold)
			{
				Polygon.RemoveAt(Index + 1);
			}
			else
			{
				Index++;
				Angle = 0.0f;
			}
		}
		return Polygon;
	}

	bool IsPointInPolygon2D(const FVector& Point, const TArray<FVector>& Polygon)
	{
		bool bInside = false;
		const int32 NumPoints = Polygon.Num();

		if (NumPoints < 3) return false;

		for (int32 i = 0, j = NumPoints - 1; i < NumPoints; j = i++)
		{
			const FVector& P1 = Polygon[i];
			const FVector& P2 = Polygon[j];

			if (Point.Y > FMath::Min(P1.Y, P2.Y))
			{
				if (Point.Y <= FMath::Max(P1.Y, P2.Y))
				{
					if (Point.X <= FMath::Max(P1.X, P2.X))
					{
						if (P1.Y != P2.Y)
						{
							float Intersection = (Point.Y - P1.Y) * (P2.X - P1.X) / (P2.Y - P1.Y) + P1.X;
							if (P1.X == P2.X || Point.X <= Intersection)
							{
								bInside = !bInside;
							}
						}
					}
				}
			}
		}

		return bInside;
	}

	FVector GetRandomPointInPolygon2D(const TArray<FVector>& Polygon, const FBox& PolygonBounds, int32 MaxAttempts)
	{
		if (Polygon.Num() < 3) return FVector::ZeroVector;

		for (int32 i = 0; i < MaxAttempts; ++i)
		{
			FVector RandomPoint = FMath::RandPointInBox(PolygonBounds);

			if (IsPointInPolygon2D(RandomPoint, Polygon))
			{
				RandomPoint.Z = Polygon[0].Z;
				return RandomPoint;
			}
		}

		return Polygon[0];
	}

	// === Sphere ===

	FBox SphereBounds(const FVector& Center, float Radius)
	{
		const FVector Extent(Radius);
		return FBox(Center - Extent, Center + Extent);
	}

	FVector ClosestPointOnSphere(const FVector& Center, float Radius, const FVector& Point)
	{
		const FVector LocalPoint = Point - Center;
		const float VSq = LocalPoint.SizeSquared();
		if (VSq <= FMath::Square(Radius)) return Point;
		return Center + LocalPoint * (Radius * FMath::InvSqrt(VSq));
	}

	bool SphereIntersectsPoint(const FVector& Center, float Radius, const FVector& Point)
	{
		return FVector::DistSquared(Point, Center) <= Radius * Radius;
	}


	// === Box ===

	FBox BoxBounds(const FVector& Center, const FQuat& Rotation, const FVector& HalfSize)
	{
		const FVector X = Rotation.GetAxisX().GetAbs() * HalfSize.X;
		const FVector Y = Rotation.GetAxisY().GetAbs() * HalfSize.Y;
		const FVector Z = Rotation.GetAxisZ().GetAbs() * HalfSize.Z;

		const FVector Extent(X.X + Y.X + Z.X, X.Y + Y.Y + Z.Y, X.Z + Y.Z + Z.Z);
		return FBox(Center - Extent, Center + Extent);
	}

	FVector ClosestPointOnBox(const FVector& Center, const FQuat& Rotation, const FVector& HalfSize, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);
		return Center + Rotation.RotateVector(LocalPoint.BoundToBox(-HalfSize, HalfSize));
	}

	bool BoxIntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& HalfSize, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);
		return
			FMath::IsWithinInclusive(LocalPoint.X, -HalfSize.X, HalfSize.X) &&
			FMath::IsWithinInclusive(LocalPoint.Y, -HalfSize.Y, HalfSize.Y) &&
			FMath::IsWithinInclusive(LocalPoint.Z, -HalfSize.Z, HalfSize.Z);
	}


	// === Capsule ===

	FBox CapsuleBounds(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight)
	{
		// Vector from center to one cap center (local +Z axis)
		const FVector Axis = Rotation.GetAxisZ();
		const FVector HalfSegment = Axis * (HalfHeight - Radius);

		// Endpoints of the capsule spine
		const FVector CapA = Center - HalfSegment;
		const FVector CapB = Center + HalfSegment;

		// Compute min/max bounds including the radius
		const FVector Min = FVector::Min(CapA, CapB) - FVector(Radius);
		const FVector Max = FVector::Max(CapA, CapB) + FVector(Radius);

		return FBox(Min, Max);
	}

	FVector ClosestPointOnCapsule(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);
		if (FMath::Abs(LocalPoint.Z) <= HalfHeight - Radius)
		{
			return Center + Rotation.RotateVector(LocalPoint.GetClampedToMaxSize2D(Radius));
		}
		else
		{
			const FVector Offset = FVector::UpVector * FMath::Sign(LocalPoint.Z) * (HalfHeight - Radius);
			return Center + Rotation.RotateVector((LocalPoint - Offset).GetClampedToMaxSize(Radius) + Offset);
		}
	}

	bool CapsuleIntersectsPoint(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);

		return
			(FMath::Abs(LocalPoint.Z) <= HalfHeight - Radius &&
			 LocalPoint.SizeSquared2D() <= FMath::Square(Radius)) ||
			FVector::DistSquared(FVector::UpVector * (HalfHeight - Radius) * FMath::Sign(LocalPoint.Z), LocalPoint) <= FMath::Square(Radius);
	}


	// === Cylinder ===

	FBox CylinderBounds(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight)
	{
		// Axis of the cylinder in world space
		const FVector Axis = Rotation.GetAxisZ();

		// Offset to top/bottom faces
		const FVector HalfSegment = Axis * HalfHeight;

		// Combine caps and radius
		const FVector Top = Center + HalfSegment;
		const FVector Bottom = Center - HalfSegment;

		const FVector Min = FVector::Min(Top, Bottom) - FVector(Radius);
		const FVector Max = FVector::Max(Top, Bottom) + FVector(Radius);

		return FBox(Min, Max);
	}

	FVector ClosestPointOnCylinder(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);

		FVector ClosestPoint = LocalPoint.GetClampedToMaxSize2D(Radius);
		ClosestPoint.Z = FMath::Clamp(LocalPoint.Z, -HalfHeight, HalfHeight);

		return Center + Rotation.RotateVector(ClosestPoint);
	}

	bool CylinderIntersectsPoint(const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Center);
		return FMath::Abs(LocalPoint.Z) <= HalfHeight && LocalPoint.SizeSquared2D() <= FMath::Square(Radius);
	}


	// === Cone ===

	FVector ConeSupportPoint(float Length, float HalfAngle, float TipRadius, bool bSphericalCap, const FVector& Direction)
	{
		return FConeSection(Length, HalfAngle, TipRadius, bSphericalCap).Support(Direction);
	}

	FBox ConeBounds(const FVector& Origin, const FQuat& Rotation, float Length, float HalfAngle, float TipRadius, bool bSphericalCap)
	{
		const FConeSection Section(Length, HalfAngle, TipRadius, bSphericalCap);
		return BoundsFromSupport(Origin, Rotation, [&Section](const FVector& Direction) { return Section.Support(Direction); });
	}

	FVector ClosestPointOnCone(const FVector& Origin, const FQuat& Rotation, float Length, float HalfAngle, float TipRadius, bool bSphericalCap, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Origin);
		const FVector Radial(0.0, LocalPoint.Y, LocalPoint.Z);
		const FVector2D Closest = FConeSection(Length, HalfAngle, TipRadius, bSphericalCap).Closest(FVector2D(LocalPoint.X, Radial.Size()));
		return Origin + Rotation.RotateVector(FromAxialPlane(Closest, Radial));
	}

	bool ConeIntersectsPoint(const FVector& Origin, const FQuat& Rotation, float Length, float HalfAngle, float TipRadius, bool bSphericalCap, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Origin);
		return FConeSection(Length, HalfAngle, TipRadius, bSphericalCap).Contains(FVector2D(LocalPoint.X, FMath::Sqrt(FMath::Square(LocalPoint.Y) + FMath::Square(LocalPoint.Z))));
	}

	FVector2D ConeRim(float Length, float HalfAngle, float TipRadius, bool bSphericalCap)
	{
		return FConeSection(Length, HalfAngle, TipRadius, bSphericalCap).Rim;
	}

	void ConeOutline(float Length, float HalfAngle, float TipRadius, bool bSphericalCap, int32 CapSegments, TArray<FVector2D>& OutOutline)
	{
		const FConeSection Section(Length, HalfAngle, TipRadius, bSphericalCap);

		OutOutline.Reset();
		OutOutline.Add(FVector2D::ZeroVector);
		if (Section.TipRadius > 0.0)
		{
			OutOutline.Add(FVector2D(0.0, Section.TipRadius));
		}
		OutOutline.Add(Section.Rim);
		if (Section.bSphericalCap)
		{
			const int32 Segments = FMath::Max(1, CapSegments);
			for (int32 Index = Segments - 1; Index > 0; --Index)
			{
				OutOutline.Add(Section.OnCap(Section.Angle * Index / Segments));
			}
		}
		OutOutline.Add(FVector2D(Section.Length, 0.0));
	}


	// === Wedge ===

	FVector WedgeSupportPoint(float Radius, float HalfAngle, float HalfHeight, const FVector& Direction)
	{
		const FVector2D Planar = FWedgeSection(Radius, HalfAngle).Support(FVector2D(Direction.X, Direction.Y));
		return FVector(Planar.X, Planar.Y, FMath::Sign(Direction.Z) * HalfHeight);
	}

	FBox WedgeBounds(const FVector& Origin, const FQuat& Rotation, float Radius, float HalfAngle, float HalfHeight)
	{
		const FWedgeSection Section(Radius, HalfAngle);
		return BoundsFromSupport(Origin, Rotation, [&Section, HalfHeight](const FVector& Direction)
		{
			const FVector2D Planar = Section.Support(FVector2D(Direction.X, Direction.Y));
			return FVector(Planar.X, Planar.Y, FMath::Sign(Direction.Z) * HalfHeight);
		});
	}

	FVector ClosestPointOnWedge(const FVector& Origin, const FQuat& Rotation, float Radius, float HalfAngle, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Origin);
		const FVector2D Planar = FWedgeSection(Radius, HalfAngle).Closest(FVector2D(LocalPoint.X, LocalPoint.Y));
		return Origin + Rotation.RotateVector(FVector(Planar.X, Planar.Y, FMath::Clamp(LocalPoint.Z, -HalfHeight, HalfHeight)));
	}

	bool WedgeIntersectsPoint(const FVector& Origin, const FQuat& Rotation, float Radius, float HalfAngle, float HalfHeight, const FVector& Point)
	{
		const FVector LocalPoint = Rotation.UnrotateVector(Point - Origin);
		return FMath::Abs(LocalPoint.Z) <= HalfHeight && FWedgeSection(Radius, HalfAngle).Contains(FVector2D(LocalPoint.X, LocalPoint.Y));
	}
}