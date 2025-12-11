// Copyright 2025 kirzo

#pragma once

#include "Containers/Array.h"
#include "Math/Box.h"
#include "Concepts/KzContainer.h"

struct FKzHitResult;
struct FKzShapeInstance;

namespace Kz
{
	/**
	 * Loose octree for broad-phase spatial queries.
	 * Supports fast raycast/overlap traversal and configurable loose bounds.
	 *
	 * When bAllowMultiNode = true (default), elements may reside in multiple child
	 * nodes if their bounds cross cell boundaries, ensuring robust queries without
	 * relying on large looseness values.
	 */
	template <typename ElementType, typename OctreeSemantics, bool bAllowMultiNode = true>
	class TOctree
	{
		using ElementIdType = typename OctreeSemantics::ElementIdType;
		using FDefaultValidator = decltype([](const ElementType&) { return true; });

	public:
		/** Sets maximum subdivision depth. */
		void SetMaxDepth(int32 InMaxDepth) { MaxDepth = FMath::Max(0, InMaxDepth); }

		/** Sets minimum number of elements per node before subdivision stops. */
		void SetMinElementsPerNode(int32 InMinElements) { MinElementsPerNode = FMath::Max(1, InMinElements); }

		/** Sets how "loose" each node’s AABB should be. Values >1 enlarge the boxes slightly to avoid precision gaps. */
		void SetLooseness(float InLooseness) { Looseness = FMath::Max(1.0f, InLooseness); }

		/** Resets the octree. */
		void Reset() { Root = FNode{}; }

		/** Builds the octree from any iterable container (Array, THandleArray, etc.). */
		void Build(const CKzContainer auto& Container);

		/**
		 * Performs a raycast through the octree using broad-phase (node AABB) and narrow-phase
		 * shape intersection tests. The semantics type determines how to obtain shapes and IDs.
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
		template<typename TValidator = FDefaultValidator>
		bool Raycast(ElementIdType& OutId, FKzHitResult& OutHit, const FVector& RayStart, const FVector& RayDir, float RayLength, TValidator&& Validator = {}) const;

		/**
		 * Performs an overlap query using a box.
		 *
		 * @param OutResults     Array receiving IDs of overlapping elements.
		 * @param Bounds         The box to query with.
		 * @param Validator      Optional callable: bool(const ElementType&).
		 */
		template<typename TValidator = FDefaultValidator>
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
		template<typename TValidator = FDefaultValidator>
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
		struct FNode
		{
			FBox Bounds;
			TArray<ElementType> Elements;
			TArray<FNode> Children;
			int32 Depth = 0;
			bool IsLeaf() const { return Children.Num() == 0; }
		};

		/** Recursively subdivides a node and distributes elements by their bounds center. */
		void BuildRecursive(FNode& N);

		/**
		 * Recursive helper for Raycast().
		 * Performs broad-phase node intersection and delegates narrow-phase tests to the validator.
		 */
		template<typename TValidator>
		void RaycastRecursive(const FNode& N, ElementIdType& OutId, FKzHitResult& OutHit, const FVector& RayStart, const FVector& RayDir, float RayLength, TValidator&& Validator, TSet<ElementIdType>& Visited) const;

		/** Recursive helper for Query(). */
		template<typename TValidator>
		void QueryRecursive(const FNode& N, TArray<ElementIdType>& OutResults, const FBox& Bounds, TValidator&& Validator, TSet<ElementIdType>& Visited) const;

		/** Recursive helper for Query(). */
		template<typename TValidator>
		void QueryRecursive(const FNode& N, TArray<ElementIdType>& OutResults, const FKzShapeInstance& Shape, const FVector& ShapePosition, const FQuat& ShapeRotation, const FBox& QueryAABB, TValidator&& Validator, TSet<ElementIdType>& Visited) const;

		static FKzShapeInstance GetElementShape(const ElementType& E);
		static FQuat GetElementRotation(const ElementType& E);

		FNode Root;
		int32 MaxDepth = 6;
		int32 MinElementsPerNode = 4;
		float Looseness = 1.0f;
	};
}

#include "Spatial/KzOctree.inl"