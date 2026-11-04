// Copyright 2026 kirzo

#include "Collision/KzGJK.h"
#include "Collision/KzHitResult.h"
#include "Math/Geometry/KzShapeInstance.h"

namespace Kz::GJK
{
	/**
	 * Simplex for GJK.
	 * NOTE: This does NOT use the classic GJK convention where the newest point
	 * is stored at index 0. Here, points are kept in insertion order and the
	 * newest support point is always Points[NumPoints - 1]. All update functions
	 * are written to follow this ordering.
	 */
	struct FSimplex
	{
		/** Adds a new Minkowski support point to the simplex. */
		void Add(const FVector& P)
		{
			check(NumPoints < 4);
			Points[NumPoints++] = P;
		}

		/** Dispatch to the appropriate simplex evolution function. */
		bool Next(FVector& Direction)
		{
			switch (NumPoints)
			{
				case 1: Direction = -Points[0]; return false;
				case 2: return Line(Direction);
				case 3: return Triangle(Direction);
				case 4: return Tetrahedron(Direction);
			}

			check(false);
			return false;
		}

	private:
		/** Handles the 1D simplex (line segment).
		 *  Returns false and updates direction to keep searching.
		 */
		bool Line(FVector& Direction)
		{
			// a = newest point, b = previous point
			const FVector& a = Points[1];
			const FVector& b = Points[0];

			const FVector ab = b - a;
			const FVector ao = -a;

			// If the origin lies beyond A in direction AB
			if (FVector::DotProduct(ab, ao) > 0.0f)
			{
				// New direction is perpendicular to AB towards the origin
				Direction = FVector::CrossProduct(FVector::CrossProduct(ab, ao), ab);
			}
			else
			{
				// Drop B, keep only A
				Points[0] = a;
				NumPoints = 1;
				Direction = ao;
			}

			return false;
		}

		/** Handles the 2D simplex (triangle). */
		bool Triangle(FVector& Direction)
		{
			// a = newest, then b, then c
			const FVector& a = Points[2];
			const FVector& b = Points[1];
			const FVector& c = Points[0];

			const FVector ab = b - a;
			const FVector ac = c - a;
			const FVector ao = -a;

			const FVector abc = FVector::CrossProduct(ab, ac);

			// Check if origin is outside AC
			if (FVector::DotProduct(FVector::CrossProduct(abc, ac), ao) > 0.0f)
			{
				if (FVector::DotProduct(ac, ao) > 0.0f)
				{
					// Reduce to line A-C
					Points[0] = a;
					Points[1] = c;
					NumPoints = 2;

					const FVector acDir = c - a;
					Direction = FVector::CrossProduct(FVector::CrossProduct(acDir, ao), acDir);
				}
				else
				{
					// Reduce to line A-B
					Points[0] = a;
					Points[1] = b;
					NumPoints = 2;
					return Line(Direction);
				}
			}
			else
			{
				// Check if origin is outside AB
				if (FVector::DotProduct(FVector::CrossProduct(ab, abc), ao) > 0.0f)
				{
					// Reduce to line A-B
					Points[0] = a;
					Points[1] = b;
					NumPoints = 2;
					return Line(Direction);
				}
				else
				{
					// Origin lies above or below the triangle
					if (FVector::DotProduct(abc, ao) > 0.0f)
					{
						// Above ABC
						Direction = abc;
					}
					else
					{
						// Below ABC, flip winding
						Points[0] = a;
						Points[1] = c;
						Points[2] = b;
						Direction = -abc;
					}
				}
			}

			return false;
		}

		/** Handles the 3D simplex (tetrahedron). */
		bool Tetrahedron(FVector& Direction)
		{
			// a = newest, then b, c, d
			const FVector& a = Points[3];
			const FVector& b = Points[2];
			const FVector& c = Points[1];
			const FVector& d = Points[0];

			const FVector ao = -a;

			const FVector ab = b - a;
			const FVector ac = c - a;
			const FVector ad = d - a;

			const FVector abc = FVector::CrossProduct(ab, ac);
			const FVector acd = FVector::CrossProduct(ac, ad);
			const FVector adb = FVector::CrossProduct(ad, ab);

			// Check face ABC
			if (FVector::DotProduct(abc, ao) > 0.0f)
			{
				Points[0] = a;
				Points[1] = b;
				Points[2] = c;
				NumPoints = 3;
				return Triangle(Direction);
			}

			// Check face ACD
			if (FVector::DotProduct(acd, ao) > 0.0f)
			{
				Points[0] = a;
				Points[1] = c;
				Points[2] = d;
				NumPoints = 3;
				return Triangle(Direction);
			}

			// Check face ADB
			if (FVector::DotProduct(adb, ao) > 0.0f)
			{
				Points[0] = a;
				Points[1] = d;
				Points[2] = b;
				NumPoints = 3;
				return Triangle(Direction);
			}

			// Origin is inside the tetrahedron
			return true;
		}

		FVector Points[4];
		int32 NumPoints = 0;
	};

	static FVector Support(const FKzShapeInstance& A, const FVector& pA, const FQuat& qA, const FKzShapeInstance& B, const FVector& pB, const FQuat& qB, const FVector& Dir)
	{
		const FVector DirLocalA = qA.UnrotateVector(Dir);
		const FVector DirLocalB = qB.UnrotateVector(-Dir);

		const FVector sA = pA + qA.RotateVector(A.GetSupportPoint(DirLocalA));
		const FVector sB = pB + qB.RotateVector(B.GetSupportPoint(DirLocalB));

		return sA - sB;
	}

	bool Raycast(FKzHitResult& OutHit, const FVector& RayOrigin, const FVector& RayDir, float MaxDistance, const FKzShapeInstance& Shape, const FVector& ShapePos, const FQuat& ShapeRot)
	{
		// First check if the shape implements a raycast function (should be way faster than GJK raycast).
		if (Shape.ImplementsRaycast())
		{
			return Shape.Raycast(OutHit, ShapePos, ShapeRot, RayOrigin, RayDir, MaxDistance);
		}

		// Fallback to generic GJK raycast

		OutHit.Reset(1.0f, false);
		OutHit.TraceStart = RayOrigin;
		OutHit.TraceEnd = RayOrigin + RayDir * MaxDistance;

		// Start penetrating?
		if (Shape.IntersectsPoint(ShapePos, ShapeRot, RayOrigin))
		{
			OutHit.bBlockingHit = true;
			OutHit.bStartPenetrating = true;
			OutHit.Location = RayOrigin;
			OutHit.Normal = -RayDir;
			OutHit.Time = 0.0f;
			OutHit.Distance = 0.0f;
			return true;
		}

		float t = 0.0f;
		FVector Current = RayOrigin;

		// Initial direction towards the shape
		FVector Dir = ShapePos - Current;
		if (Dir.IsNearlyZero())
		{
			Dir = -RayDir;
		}

		FSimplex Simplex;

		while (t <= MaxDistance)
		{
			// Minkowski support: shape - point(Current)
			FVector LocalDir = ShapeRot.UnrotateVector(-Dir);
			FVector SupportS = ShapePos + ShapeRot.RotateVector(Shape.GetSupportPoint(LocalDir));
			FVector SupportPoint = SupportS - Current;

			float Dot = FVector::DotProduct(SupportPoint, Dir);
			if (Dot < 0.0f)
			{
				return false; // miss
			}

			Simplex.Add(SupportPoint);

			// GJK simplex evolution
			if (Simplex.Next(Dir))
			{
				OutHit.bBlockingHit = true;
				OutHit.bStartPenetrating = false;
				OutHit.Time = t / MaxDistance;
				OutHit.Distance = t;
				OutHit.Location = Current;
				OutHit.Normal = Dir.GetSafeNormal();
				return true;
			}

			// Conservative advancement step
			float Step = Dot / FVector::DotProduct(Dir, RayDir);
			if (Step <= UE_KINDA_SMALL_NUMBER)
			{
				return false;
			}

			t += Step;
			Current = RayOrigin + RayDir * t;
		}

		return false;
	}

	bool Intersect(const FKzShapeInstance& A, const FVector& pA, const FQuat& qA, const FKzShapeInstance& B, const FVector& pB, const FQuat& qB, int32 MaxIterations)
	{
		// Early exit: Check whether ShapeA origin is inside ShapeB and viceversa.
		if (A.IntersectsPoint(pA, qA, pB) || B.IntersectsPoint(pB, qB, pA))
		{
			return true;
		}

		// This direction could be random.
		FVector Dir = FVector::OneVector;

		FVector SupportPoint = Support(A, pA, qA, B, pB, qB, Dir);

		FSimplex Simplex;
		Simplex.Add(SupportPoint);

		Dir = -SupportPoint;

		for (int32 i = MaxIterations; --i;)
		{
			SupportPoint = Support(A, pA, qA, B, pB, qB, Dir);

			if (FVector::DotProduct(SupportPoint, Dir) < UE_KINDA_SMALL_NUMBER)
			{
				return false; // No intersection
			}

			Simplex.Add(SupportPoint);

			if (Simplex.Next(Dir))
			{
				return true;
			}
		}

		return false;
	}
}