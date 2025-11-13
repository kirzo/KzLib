// Copyright 2025 kirzo

#include "Collision/KzRaycast.h"
#include "Collision/KzHitResult.h"

namespace Kz::Raycast
{
	bool Sphere(FKzHitResult& OutHit, const FVector& Center, float Radius, const FVector& RayStart, const FVector& RayDir, float MaxDistance)
	{
		OutHit.Reset(1.f, false);
		OutHit.TraceStart = RayStart;
		OutHit.TraceEnd = RayStart + RayDir * (MaxDistance <= 0 ? UE_BIG_NUMBER : MaxDistance);

		const FVector m = RayStart - Center;

		// Optional optimization
		check(RayDir.IsNormalized());
		const float b = FVector::DotProduct(m, RayDir);
		const float c = FVector::DotProduct(m, m) - Radius * Radius;
		const float disc = b * b - c;

		if (disc < 0.0f)
			return false;

		const float sqrtDisc = FMath::Sqrt(disc);

		const float t1 = -b - sqrtDisc;
		const float t2 = -b + sqrtDisc;

		float t = t1;

		// If t1 is negative, we start inside the sphere
		if (t1 < 0.0f)
		{
			OutHit.bStartPenetrating = true;
			t = t2;

			if (t2 < 0.0f)
			{
				// Ray starts **deep inside** and points away, treat as impact at origin
				OutHit.bBlockingHit = true;
				OutHit.Time = 0.f;
				OutHit.Distance = 0.f;
				OutHit.Location = RayStart;
				OutHit.Normal = (RayStart - Center).GetSafeNormal();
				return true;
			}
		}

		if (MaxDistance > 0.0f && t > MaxDistance)
			return false;

		OutHit.bBlockingHit = true;

		if (MaxDistance > 0)
			OutHit.Time = t / MaxDistance;
		else
			OutHit.Time = 0.f;

		OutHit.Distance = t;
		OutHit.Location = RayStart + RayDir * t;
		OutHit.Normal = (OutHit.Location - Center).GetSafeNormal();

		return true;
	}

	bool Box(FKzHitResult& OutHit, const FVector& Center, const FVector& Extents, const FVector& RayStart, const FVector& RayDir, float MaxDistance)
	{
		if (MaxDistance <= 0.0f)
		{
			MaxDistance = UE_BIG_NUMBER;
		}

		OutHit.Reset(1.f, false);
		OutHit.TraceStart = RayStart;
		OutHit.TraceEnd = RayStart + RayDir * MaxDistance;

		// Compute AABB min/max
		const FVector Min = Center - Extents;
		const FVector Max = Center + Extents;

		float tmin = 0.f;
		float tmax = MaxDistance;

		// X slab
		if (FMath::Abs(RayDir.X) < UE_SMALL_NUMBER)
		{
			if (RayStart.X < Min.X || RayStart.X > Max.X)
				return false;
		}
		else
		{
			float tx1 = (Min.X - RayStart.X) / RayDir.X;
			float tx2 = (Max.X - RayStart.X) / RayDir.X;
			if (tx1 > tx2) Swap(tx1, tx2);
			tmin = FMath::Max(tmin, tx1);
			tmax = FMath::Min(tmax, tx2);
			if (tmin > tmax) return false;
		}

		// Y slab
		if (FMath::Abs(RayDir.Y) < UE_SMALL_NUMBER)
		{
			if (RayStart.Y < Min.Y || RayStart.Y > Max.Y)
				return false;
		}
		else
		{
			float ty1 = (Min.Y - RayStart.Y) / RayDir.Y;
			float ty2 = (Max.Y - RayStart.Y) / RayDir.Y;
			if (ty1 > ty2) Swap(ty1, ty2);
			tmin = FMath::Max(tmin, ty1);
			tmax = FMath::Min(tmax, ty2);
			if (tmin > tmax) return false;
		}

		// Z slab
		if (FMath::Abs(RayDir.Z) < UE_SMALL_NUMBER)
		{
			if (RayStart.Z < Min.Z || RayStart.Z > Max.Z)
				return false;
		}
		else
		{
			float tz1 = (Min.Z - RayStart.Z) / RayDir.Z;
			float tz2 = (Max.Z - RayStart.Z) / RayDir.Z;
			if (tz1 > tz2) Swap(tz1, tz2);
			tmin = FMath::Max(tmin, tz1);
			tmax = FMath::Min(tmax, tz2);
			if (tmin > tmax) return false;
		}

		// Final t
		const float t = (tmin >= 0.f ? tmin : tmax);
		if (t < 0.f || t > MaxDistance)
			return false;

		// Fill hit data
		OutHit.bBlockingHit = true;
		OutHit.Distance = t;
		OutHit.Time = t / MaxDistance;
		OutHit.Location = RayStart + RayDir * t;

		// Compute normal using hit point
		const FVector P = OutHit.Location;

		if (FMath::IsNearlyEqual(P.X, Min.X)) OutHit.Normal = FVector(-1, 0, 0);
		else if (FMath::IsNearlyEqual(P.X, Max.X)) OutHit.Normal = FVector(1, 0, 0);
		else if (FMath::IsNearlyEqual(P.Y, Min.Y)) OutHit.Normal = FVector(0, -1, 0);
		else if (FMath::IsNearlyEqual(P.Y, Max.Y)) OutHit.Normal = FVector(0, 1, 0);
		else if (FMath::IsNearlyEqual(P.Z, Min.Z)) OutHit.Normal = FVector(0, 0, -1);
		else                                      OutHit.Normal = FVector(0, 0, 1);

		return true;
	}

	bool Box(FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& Extents, const FVector& RayStart, const FVector& RayDir, float MaxDistance)
	{
		if (MaxDistance <= 0.0f)
		{
			MaxDistance = UE_BIG_NUMBER;
		}

		OutHit.Reset(1.f, false);
		OutHit.TraceStart = RayStart;
		OutHit.TraceEnd = RayStart + RayDir * MaxDistance;

		// Transform ray to local box space
		const FVector O = Rotation.UnrotateVector(RayStart - Center);
		const FVector D = Rotation.UnrotateVector(RayDir);

		float tmin = 0.f;
		float tmax = MaxDistance;

		for (int i = 0; i < 3; ++i)
		{
			const float o = O[i];
			const float d = D[i];
			const float e = Extents[i];

			if (FMath::Abs(d) < 1e-6f)
			{
				if (o < -e || o >  e)
					return false;
				continue;
			}

			float t1 = (-e - o) / d;
			float t2 = (e - o) / d;
			if (t1 > t2) Swap(t1, t2);

			tmin = FMath::Max(tmin, t1);
			tmax = FMath::Min(tmax, t2);

			if (tmin > tmax)
				return false;
		}

		const float t = (tmin >= 0.f ? tmin : tmax);
		if (t < 0.f || t > MaxDistance)
			return false;

		OutHit.Location = RayStart + RayDir * t;
		OutHit.Time = t / MaxDistance;
		OutHit.Distance = t;
		OutHit.bBlockingHit = true;

		// Determine normal by examining local hit point
		const FVector P = O + D * t;
		const FVector E = Extents;
		FVector LocalNormal(0, 0, 0);
		const float bias = 1e-4f;

		if (FMath::Abs(P.X - E.X) < bias)      LocalNormal = FVector(1, 0, 0);
		else if (FMath::Abs(P.X + E.X) < bias) LocalNormal = FVector(-1, 0, 0);
		else if (FMath::Abs(P.Y - E.Y) < bias) LocalNormal = FVector(0, 1, 0);
		else if (FMath::Abs(P.Y + E.Y) < bias) LocalNormal = FVector(0, -1, 0);
		else if (FMath::Abs(P.Z - E.Z) < bias) LocalNormal = FVector(0, 0, 1);
		else                                   LocalNormal = FVector(0, 0, -1);

		OutHit.Normal = Rotation.RotateVector(LocalNormal);

		return true;
	}

	bool Capsule(FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& RayStart, const FVector& RayDir, float MaxDistance)
	{
		if (MaxDistance <= 0.0f)
		{
			MaxDistance = UE_BIG_NUMBER;
		}

		OutHit.Reset(1.f, false);
		OutHit.TraceStart = RayStart;
		OutHit.TraceEnd = RayStart + RayDir * MaxDistance;

		// Compute capsule hemisphere centers in world space
		const float CylinderHalf = HalfHeight - Radius;

		const FVector A = Center + Rotation.RotateVector(FVector(0, 0, CylinderHalf));
		const FVector B = Center + Rotation.RotateVector(FVector(0, 0, -CylinderHalf));

		// Degenerate: treat as sphere
		const FVector AB = B - A;
		const float AB2 = AB.SizeSquared();
		if (AB2 < UE_KINDA_SMALL_NUMBER)
		{
			return Sphere(OutHit, Center, Radius, RayStart, RayDir, MaxDistance);
		}

		const FVector AO = RayStart - A;
		const FVector D = RayDir;

		// Swept sphere vs segment
		const FVector ABxD = FVector::CrossProduct(AB, D);
		const FVector ABxAO = FVector::CrossProduct(AB, AO);

		const float a = ABxD.SizeSquared();
		const float b = 2.f * FVector::DotProduct(ABxD, ABxAO);
		const float c = ABxAO.SizeSquared() - Radius * Radius * AB2;

		const float disc = b * b - 4.f * a * c;
		if (disc < 0.f)
			return false;

		const float sqrtDisc = FMath::Sqrt(disc);
		float t0 = (-b - sqrtDisc) / (2.f * a);
		float t1 = (-b + sqrtDisc) / (2.f * a);
		if (t0 > t1) Swap(t0, t1);

		float t = (t0 >= 0.f ? t0 : t1);
		if (t < 0.f || t > MaxDistance)
			return false;

		// Hit
		OutHit.Time = t / MaxDistance;
		OutHit.Distance = t;
		OutHit.Location = RayStart + RayDir * t;

		// Normal = from closest point on AB to hit point
		float s = FVector::DotProduct(OutHit.Location - A, AB) / AB2;
		s = FMath::Clamp(s, 0.f, 1.f);

		const FVector Closest = A + s * AB;
		OutHit.Normal = (OutHit.Location - Closest).GetSafeNormal();

		OutHit.bBlockingHit = true;
		return true;
	}

	bool Cylinder(FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, float Radius, float HalfHeight, const FVector& RayStart, const FVector& RayDir, float MaxDistance)
	{
		if (MaxDistance <= 0.0f)
		{
			MaxDistance = UE_BIG_NUMBER;
		}

		OutHit.Reset(1.f, false);
		OutHit.TraceStart = RayStart;
		OutHit.TraceEnd = RayStart + RayDir * MaxDistance;

		// Transform ray to cylinder local space
		const FVector O = Rotation.UnrotateVector(RayStart - Center);
		const FVector D = Rotation.UnrotateVector(RayDir);

		float tHit = FLT_MAX;

		// Initialized to safe defaults just to silence compiler warnings.
		// They will always be overwritten before use if tHit is updated.
		FVector HitLocal(ForceInitToZero);
		FVector NormalLocal(ForceInitToZero);

		const float R = Radius;
		const float H = HalfHeight;

		// ---- Top & bottom caps ----
		for (int cap = -1; cap <= 1; cap += 2)
		{
			const float zPlane = cap * H;

			if (FMath::Abs(D.Z) > 1e-6f)
			{
				const float t = (zPlane - O.Z) / D.Z;
				if (t >= 0.f && t <= MaxDistance)
				{
					const FVector P = O + D * t;
					if ((P.X * P.X + P.Y * P.Y) <= R * R && t < tHit)
					{
						tHit = t;
						HitLocal = P;
						NormalLocal = FVector(0, 0, cap);
					}
				}
			}
		}

		// ---- Lateral surface ----
		{
			const float a = D.X * D.X + D.Y * D.Y;
			const float b = 2.f * (O.X * D.X + O.Y * D.Y);
			const float c = O.X * O.X + O.Y * O.Y - R * R;

			const float disc = b * b - 4 * a * c;
			if (a > 1e-6f && disc >= 0.f)
			{
				const float sqrtDisc = FMath::Sqrt(disc);
				float t0 = (-b - sqrtDisc) / (2 * a);
				float t1 = (-b + sqrtDisc) / (2 * a);
				if (t0 > t1) Swap(t0, t1);

				float candidates[2] = { t0, t1 };
				for (float t : candidates)
				{
					if (t >= 0.f && t <= MaxDistance)
					{
						const FVector P = O + D * t;

						if (P.Z >= -H && P.Z <= H)
						{
							if (t < tHit)
							{
								tHit = t;
								HitLocal = P;
								NormalLocal = FVector(P.X, P.Y, 0).GetSafeNormal();
							}
						}
					}
				}
			}
		}

		if (tHit == FLT_MAX)
			return false;

		OutHit.Location = Center + Rotation.RotateVector(HitLocal);
		OutHit.Normal = Rotation.RotateVector(NormalLocal);

		OutHit.Time = tHit / MaxDistance;
		OutHit.Distance = tHit;
		OutHit.bBlockingHit = true;

		return true;
	}
}