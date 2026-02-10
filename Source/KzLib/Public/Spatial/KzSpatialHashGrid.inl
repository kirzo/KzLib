// Copyright 2025 kirzo

#include "KzSpatialHashGrid.h"

#include "Collision/KzHitResult.h"
#include "Collision/KzRaycast.h"
#include "Collision/KzGJK.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "Math/Geometry/Shapes/KzSphere.h"

#include "DrawDebugHelpers.h"

namespace Kz
{
	template <typename ElementType, typename GridSemantics>
	void TSpatialHashGrid<ElementType, GridSemantics>::Build(const CKzContainer auto& Container)
	{
		Reset();

		int32 Num = Container.Num();
		if (Num == 0)
			return;

		for (const ElementType& E : Container)
		{
			Insert(E);
		}
	}

	template <typename ElementType, typename GridSemantics>
	void TSpatialHashGrid<ElementType, GridSemantics>::Insert(const ElementType& E)
	{
		const FBox Bounds = GridSemantics::GetBoundingBox(E);
		const FInt64Vector Min = GetCellCoord(Bounds.Min, CellSize);
		const FInt64Vector Max = GetCellCoord(Bounds.Max, CellSize);

		for (int64 x = Min.X; x <= Max.X; ++x)
		{
			for (int64 y = Min.Y; y <= Max.Y; ++y)
			{
				for (int64 z = Min.Z; z <= Max.Z; ++z)
				{
					uint64 Key = GetCellKey(x, y, z);
					GridCells.FindOrAdd(Key).Add(E);
				}
			}
		}
	}

	template <typename ElementType, typename GridSemantics>
	void TSpatialHashGrid<ElementType, GridSemantics>::Remove(const ElementType& E)
	{
		if (!GridSemantics::IsValid(E)) return;

		const ElementIdType IdToRemove = GridSemantics::GetElementId(E);

		// Brute force iteration over all map buckets
		for (auto It = GridCells.CreateIterator(); It; ++It)
		{
			TArray<ElementType>& Cell = It.Value();
			bool bFoundInCell = false;

			for (int32 i = 0; i < Cell.Num(); ++i)
			{
				if (GridSemantics::GetElementId(Cell[i]) == IdToRemove)
				{
					Cell.RemoveAtSwap(i, 1, false);
					bFoundInCell = true;
					break; // Found in this cell, move to next
				}
			}

			// Clean up empty buckets to keep map size manageable
			if (Cell.IsEmpty())
			{
				It.RemoveCurrent();
			}

			// Note: We DO NOT break the outer loop (It) because the object 
			// likely exists in multiple cells. We must check them all.
		}
	}

	template <typename ElementType, typename GridSemantics>
	void TSpatialHashGrid<ElementType, GridSemantics>::Remove(const ElementType& E, const FBox& PreviousBounds)
	{
		// To remove efficiently, we look only in the cells covered by the Old Bounds.
		const FInt64Vector Min = GetCellCoord(PreviousBounds.Min, CellSize);
		const FInt64Vector Max = GetCellCoord(PreviousBounds.Max, CellSize);

		const ElementIdType IdToRemove = GridSemantics::GetElementId(E);

		for (int64 x = Min.X; x <= Max.X; ++x)
		{
			for (int64 y = Min.Y; y <= Max.Y; ++y)
			{
				for (int64 z = Min.Z; z <= Max.Z; ++z)
				{
					uint64 Key = GetCellKey(x, y, z);
					if (TArray<ElementType>* Cell = GridCells.Find(Key))
					{
						// Find element by ID in this cell and remove it
						for (int32 i = 0; i < Cell->Num(); ++i)
						{
							if (GridSemantics::GetElementId((*Cell)[i]) == IdToRemove)
							{
								Cell->RemoveAtSwap(i, 1, EAllowShrinking::No);
								break; // Assuming object is only once per cell
							}
						}

						// Clean up empty cells to save memory
						if (Cell->IsEmpty())
						{
							GridCells.Remove(Key);
						}
					}
				}
			}
		}
	}

	template <typename ElementType, typename GridSemantics>
	template <typename TValidator>
	bool TSpatialHashGrid<ElementType, GridSemantics>::Raycast(ElementIdType& OutId, FKzHitResult& OutHit, const FVector& RayStart, const FVector& RayDir, float RayLength, TValidator&& Validator) const
	{
		const float SizeSq = RayDir.SizeSquared();
		if (SizeSq < UE_SMALL_NUMBER)
			return false;

		FVector Dir = RayDir;
		if (!FMath::IsNearlyEqual(SizeSq, 1.0f))
		{
			Dir *= FMath::InvSqrt(SizeSq);
		}

		if (RayLength <= 0.0f)
			RayLength = UE_BIG_NUMBER;

		OutHit.Init(RayStart, RayStart + Dir * RayLength);
		OutHit.bBlockingHit = false;
		OutHit.Distance = RayLength;

		TSet<ElementIdType> Visited;

		// DDA / Grid Traversal
		FInt64Vector Current = GetCellCoord(RayStart, CellSize);

		// Determine step direction and tMax/tDelta
		int64 StepX = (Dir.X >= 0) ? 1 : -1;
		int64 StepY = (Dir.Y >= 0) ? 1 : -1;
		int64 StepZ = (Dir.Z >= 0) ? 1 : -1;

		float tMaxX = (Dir.X != 0) ? ((Current.X + (StepX > 0 ? 1 : 0)) * CellSize - RayStart.X) / Dir.X : UE_BIG_NUMBER;
		float tMaxY = (Dir.Y != 0) ? ((Current.Y + (StepY > 0 ? 1 : 0)) * CellSize - RayStart.Y) / Dir.Y : UE_BIG_NUMBER;
		float tMaxZ = (Dir.Z != 0) ? ((Current.Z + (StepZ > 0 ? 1 : 0)) * CellSize - RayStart.Z) / Dir.Z : UE_BIG_NUMBER;

		// Correct tDelta calculation: it's distance along ray to cross one CellSize in that dimension
		float tDeltaX = (Dir.X != 0) ? CellSize / FMath::Abs(Dir.X) : UE_BIG_NUMBER;
		float tDeltaY = (Dir.Y != 0) ? CellSize / FMath::Abs(Dir.Y) : UE_BIG_NUMBER;
		float tDeltaZ = (Dir.Z != 0) ? CellSize / FMath::Abs(Dir.Z) : UE_BIG_NUMBER;

		const float LimitDist = RayLength;
		float CurrentDist = 0.0f;

		// Limit iterations to prevent infinite loops in bad cases
		int32 MaxSteps = 10000;

		while (CurrentDist <= LimitDist && MaxSteps-- > 0)
		{
			uint64 Key = GetCellKey(Current.X, Current.Y, Current.Z);
			const TArray<ElementType>* Cell = GridCells.Find(Key);

			if (Cell)
			{
				for (const ElementType& E : *Cell)
				{
					const ElementIdType Id = GridSemantics::GetElementId(E);
					if (Visited.Contains(Id))
						continue;
					Visited.Add(Id);

					if (!GridSemantics::IsValid(E) || !Validator(E))
						continue;

					const FKzShapeInstance ElemShape = GetElementShape(E);
					const FVector ElemPos = GridSemantics::GetElementPosition(E);
					const FQuat ElemRot = GetElementRotation(E);

					const float MaxCheckLength = OutHit.bBlockingHit ? OutHit.Distance : RayLength;
					const float PrevDist = OutHit.Distance;

					FKzHitResult HitCandidate = OutHit;
					if (Kz::GJK::Raycast(HitCandidate, RayStart, Dir, MaxCheckLength, ElemShape, ElemPos, ElemRot) && HitCandidate.Distance < PrevDist)
					{
						OutHit = HitCandidate;
						OutId = Id;
					}
				}
			}

			// If we found a hit, check if we can stop.
			// Ideally we stop if CurrentDist > OutHit.Distance.
			// But since objects are larger than cells, we might find a hit in Cell A
			// that is actually further away than a potential hit in Cell B. However,
			// standard spatial has traversal visits cells front-to-back. The only risk
			// is if an object in Cell A has geometry that is actually in Cell B
			// (overlaps bound). But we insert objects into ALL cells they overlap. So
			// if geometry is in Cell B, the object IS in Cell B. So if we find a hit,
			// we must ensure we process all cells up to the hit distance.
			if (OutHit.bBlockingHit && OutHit.Distance < CurrentDist)
			{
				break;
			}
			float const NewLimit = OutHit.bBlockingHit ? OutHit.Distance : RayLength;

			// Advance to next voxel
			if (tMaxX < tMaxY)
			{
				if (tMaxX < tMaxZ)
				{
					if (tMaxX > NewLimit)
						break;
					CurrentDist = tMaxX;
					Current.X += StepX;
					tMaxX += tDeltaX;
				}
				else
				{
					if (tMaxZ > NewLimit)
						break;
					CurrentDist = tMaxZ;
					Current.Z += StepZ;
					tMaxZ += tDeltaZ;
				}
			}
			else
			{
				if (tMaxY < tMaxZ)
				{
					if (tMaxY > NewLimit)
						break;
					CurrentDist = tMaxY;
					Current.Y += StepY;
					tMaxY += tDeltaY;
				}
				else
				{
					if (tMaxZ > NewLimit)
						break;
					CurrentDist = tMaxZ;
					Current.Z += StepZ;
					tMaxZ += tDeltaZ;
				}
			}
		}

		return OutHit.bBlockingHit;
	}

	template <typename ElementType, typename GridSemantics>
	template <typename TValidator>
	bool TSpatialHashGrid<ElementType, GridSemantics>::Query(TArray<ElementIdType>& OutResults, const FBox& Bounds, TValidator&& Validator) const
	{
		TSet<ElementIdType> Visited;

		const FInt64Vector Min = GetCellCoord(Bounds.Min, CellSize);
		const FInt64Vector Max = GetCellCoord(Bounds.Max, CellSize);

		for (int64 x = Min.X; x <= Max.X; ++x)
		{
			for (int64 y = Min.Y; y <= Max.Y; ++y)
			{
				for (int64 z = Min.Z; z <= Max.Z; ++z)
				{
					uint64 Key = GetCellKey(x, y, z);
					const TArray<ElementType>* Cell = GridCells.Find(Key);
					if (!Cell)
						continue;

					for (const ElementType& E : *Cell)
					{
						const ElementIdType Id = GridSemantics::GetElementId(E);
						if (Visited.Contains(Id))
							continue;
						Visited.Add(Id);

						if (!GridSemantics::IsValid(E) || !Validator(E))
							continue;

						if (Bounds.Intersect(GridSemantics::GetBoundingBox(E)))
						{
							OutResults.Add(Id);
						}
					}
				}
			}
		}

		return !OutResults.IsEmpty();
	}

	template <typename ElementType, typename GridSemantics>
	template <typename TValidator>
	bool TSpatialHashGrid<ElementType, GridSemantics>::Query(TArray<ElementIdType>& OutResults, const FKzShapeInstance& Shape, const FVector& ShapePosition, const FQuat& ShapeRotation, TValidator&& Validator) const
	{
		const FBox QueryAABB = Shape.GetBoundingBox(ShapePosition, ShapeRotation);
		if (!QueryAABB.IsValid)
			return false;

		TSet<ElementIdType> Visited;
		const FInt64Vector Min = GetCellCoord(QueryAABB.Min, CellSize);
		const FInt64Vector Max = GetCellCoord(QueryAABB.Max, CellSize);

		for (int64 x = Min.X; x <= Max.X; ++x)
		{
			for (int64 y = Min.Y; y <= Max.Y; ++y)
			{
				for (int64 z = Min.Z; z <= Max.Z; ++z)
				{
					uint64 Key = GetCellKey(x, y, z);
					const TArray<ElementType>* Cell = GridCells.Find(Key);
					if (!Cell)
						continue;

					for (const ElementType& E : *Cell)
					{
						const ElementIdType Id = GridSemantics::GetElementId(E);
						if (Visited.Contains(Id))
							continue;
						Visited.Add(Id);

						if (!GridSemantics::IsValid(E) || !Validator(E))
							continue;
						if (!QueryAABB.Intersect(GridSemantics::GetBoundingBox(E)))
							continue;

						const FKzShapeInstance ElemShape = GetElementShape(E);
						const FVector ElemPos = GridSemantics::GetElementPosition(E);
						const FQuat ElemRot = GetElementRotation(E);

						if (Kz::GJK::Intersect(Shape, ShapePosition, ShapeRotation, ElemShape, ElemPos, ElemRot))
						{
							OutResults.Add(Id);
						}
					}
				}
			}
		}

		return !OutResults.IsEmpty();
	}

	template <typename ElementType, typename GridSemantics>
	void TSpatialHashGrid<ElementType, GridSemantics>::DebugDraw(const UWorld* World, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
	{
		if (!World)
			return;

		for (const auto& [Key, Elements] : GridCells)
		{
			if (Elements.IsEmpty())
				continue;

			// Decode Key
			// 21 bits per component.
			// Need to handle sign extension since we packed int64 into uint64 bits.
			// Logic: We packed as (Value + Offset) to be positive, OR we just mask bits
			// and treat as signed 21-bit int.
			int64 x = (Key) & 0x1FFFFF;
			int64 y = (Key >> 21) & 0x1FFFFF;
			int64 z = (Key >> 42) & 0x1FFFFF;

			// Restore sign (21st bit is sign bit equivalent in 2's complement if we just masked, effectively)
			if (x & 0x100000) x |= 0xFFFFFFFFFFE00000;
			if (y & 0x100000) y |= 0xFFFFFFFFFFE00000;
			if (z & 0x100000) z |= 0xFFFFFFFFFFE00000;

			FVector Center((float)x * CellSize + CellSize * 0.5f,
										 (float)y * CellSize + CellSize * 0.5f,
										 (float)z * CellSize + CellSize * 0.5f);
			FVector Extent(CellSize * 0.5f);

			DrawDebugBox(World, Center, Extent, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		}
	}

	// Helpers
	template <typename ElementType, typename GridSemantics>
	uint64 TSpatialHashGrid<ElementType, GridSemantics>::GetCellKey(int64 X, int64 Y, int64 Z)
	{
		// Packed Key for reversibility (21 bits per axis)
		// Mask to 21 bits: 0x1FFFFF
		// Support range approx +/- 1 million cells.
		// If CellSize=100cm (1m), cover +/- 1000km. Sufficient.

		uint64 kX = (uint64)(X & 0x1FFFFF);
		uint64 kY = (uint64)(Y & 0x1FFFFF);
		uint64 kZ = (uint64)(Z & 0x1FFFFF);

		return kX | (kY << 21) | (kZ << 42);
	}

	template <typename ElementType, typename GridSemantics>
	FInt64Vector TSpatialHashGrid<ElementType, GridSemantics>::GetCellCoord(const FVector& Pos, float CellSize)
	{
		return FInt64Vector{ (int64)FMath::FloorToInt(Pos.X / CellSize),
												 (int64)FMath::FloorToInt(Pos.Y / CellSize),
												 (int64)FMath::FloorToInt(Pos.Z / CellSize) };
	}

	template <typename ElementType, typename GridSemantics>
	FKzShapeInstance TSpatialHashGrid<ElementType, GridSemantics>::GetElementShape(const ElementType& E)
	{
		if constexpr (requires { GridSemantics::GetShape(E); })
		{
			return GridSemantics::GetShape(E);
		}
		else
		{
			// Fallback: use bounding sphere derived from bounding box.
			const FBox B = GridSemantics::GetBoundingBox(E);
			return FKzShapeInstance::Make<FKzSphere>(B.GetExtent().GetAbsMax());
		}
	}

	template <typename ElementType, typename GridSemantics>
	FQuat TSpatialHashGrid<ElementType, GridSemantics>::GetElementRotation(const ElementType& E)
	{
		if constexpr (requires { GridSemantics::GetElementRotation(E); })
		{
			return GridSemantics::GetElementRotation(E);
		}
		else
		{
			return FQuat::Identity;
		}
	}
}