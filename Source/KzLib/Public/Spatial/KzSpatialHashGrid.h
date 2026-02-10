// Copyright 2025 kirzo

#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Math/Box.h"
#include "Concepts/KzContainer.h"

struct FKzHitResult;
struct FKzShapeInstance;

namespace Kz
{
	/**
	 * Sparse spatial hash grid for broad-phase spatial queries.
	 * Uses a TMap to store partial infinite grid cells.
	 * Excellent for unbounded worlds or when objects are sparsely distributed.
	 */
	template <typename ElementType, typename GridSemantics>
	class TSpatialHashGrid
	{
		using ElementIdType = typename GridSemantics::ElementIdType;
		using FDefaultValidator = decltype([](const ElementType&) { return true; });

	public:
		/** Sets the cell size of the grid. Larger cells mean broader broad-phase but more narrow-phase checks. */
		void SetCellSize(float InCellSize) { CellSize = FMath::Max(1.0f, InCellSize); }

		/** Resets the grid. */
		void Reset()
		{
			for (auto& Pair : GridCells)
			{
				Pair.Value.Reset();
			}
		}

		/** Builds the octree from any iterable container (Array, THandleArray, etc.). */
		void Build(const CKzContainer auto& Container);

		/** Inserts a single element into the grid (O(1)). */
		void Insert(const ElementType& Element);

		/** 
		 * Removes a single element from the grid by searching ALL active cells (O(N)).
		 * WARNING: Slower than Remove(Element, Bounds). Use with caution.
		 * @param Element The element to remove.
		 */
		void Remove(const ElementType& Element);

		/**
		 * Removes a single element from the grid (O(1)).
		 * Requires the Bounds where the object was located to find it efficiently.
		 */
		void Remove(const ElementType& Element, const FBox& PreviousBounds);

		/**
		 * Performs a raycast through the grid using fast voxel traversal (DDA).
		 * 
		 * The validator (optional) allows filtering elements (eg. collision filtering).
		 *
		 * @param OutId         Receives the ID of the closest intersected element.
		 * @param OutHit        Receives geometric hit information (distance, location, normal...).
		 * @param RayStart      Ray world-space start position.
		 * @param RayDir        Ray direction (does not need to be normalized).
		 * @param RayLength     Ray length. <= 0 means infinite.
		 * @param Validator     Optional callable: bool(const ElementType&)
		 * @return true if any element was hit; false otherwise.
		 */
		template <typename TValidator = FDefaultValidator>
		bool Raycast(ElementIdType& OutId, FKzHitResult& OutHit, const FVector& RayStart, const FVector& RayDir, float RayLength, TValidator&& Validator = {}) const;

		/**
		 * Performs an overlap query using a box.
		 *
		 * @param OutResults     Array receiving IDs of overlapping elements.
		 * @param Bounds         The box to query with.
		 * @param Validator      Optional callable: bool(const ElementType&).
		 */
		template <typename TValidator = FDefaultValidator>
		bool Query(TArray<ElementIdType>& OutResults, const FBox& Bounds, TValidator&& Validator = {}) const;

		/**
		 * Performs an overlap query using a shape.
		 *
		 * @param OutResults     Array receiving IDs of overlapping elements.
		 * @param Shape          The geometric shape definition to query with.
		 * @param ShapePosition  World-space position of the shape.
		 * @param ShapeRotation  World-space orientation of the shape.
		 * @param Validator      Optional callable: bool(const ElementType&).
		 */
		template <typename TValidator = FDefaultValidator>
		bool Query(TArray<ElementIdType>& OutResults, const FKzShapeInstance& Shape, const FVector& ShapePosition, const FQuat& ShapeRotation, TValidator&& Validator = {}) const;

		/**
		 * Draws a debug visualization.
		 *
		 * @param World            The world where debug lines will be drawn.
		 * @param Color            Color of the box outlines.
		 * @param bPersistentLines If true, lines stay on screen until cleared.
		 * @param LifeTime         How long (in seconds) lines should persist (ignored if bPersistentLines=true).
		 * @param DepthPriority    Drawing priority (see ESceneDepthPriorityGroup).
		 * @param Thickness        Line thickness.
		 */
		void DebugDraw(const class UWorld* World, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) const;

	private:
		static uint64 GetCellKey(int64 X, int64 Y, int64 Z);
		static FInt64Vector GetCellCoord(const FVector& Pos, float CellSize);

		static FKzShapeInstance GetElementShape(const ElementType& E);
		static FQuat GetElementRotation(const ElementType& E);

		TMap<uint64, TArray<ElementType>> GridCells;
		float CellSize = 100.0f;
	};
}

#include "Spatial/KzSpatialHashGrid.inl"