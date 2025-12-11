// Copyright 2025 kirzo

#include "KzOctree.h"

#include "Collision/KzHitResult.h"
#include "Collision/KzRaycast.h"
#include "Collision/KzGJK.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "Math/Geometry/Shapes/KzSphere.h"

#include "DrawDebugHelpers.h"

namespace Kz
{
	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	void TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::Build(const CKzContainer auto& Container)
	{
		Reset();

		int32 Num = Container.Num();
		if (Num == 0)
			return;

		// Compute global bounds
		FBox Global(ForceInitToZero);
		for (const ElementType& E : Container)
		{
			Global += OctreeSemantics::GetBoundingBox(E);
		}

		// Make cubic + small pad for robustness
		const FVector Center = Global.GetCenter();
		const FVector HalfSize(Global.GetExtent().GetMax());
		const FVector PadHalf = HalfSize * 1.02f;
		Root.Bounds = FBox(Center - PadHalf, Center + PadHalf);
		Root.Depth = 0;

		// Fill root node
		Root.Elements.Reserve(Num);
		for (const ElementType& E : Container)
		{
			Root.Elements.Add(E);
		}

		// Subdivide
		BuildRecursive(Root);
	}

	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	void TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::BuildRecursive(FNode& N)
	{
		// Stop if reached limits
		if (N.Depth >= MaxDepth || N.Elements.Num() <= MinElementsPerNode)
		{
			return; // Leaf
		}

		// This node is not a leaf, so subdivide it into 8 children.
		N.Children.SetNum(8);

		const FVector ParentCenter = N.Bounds.GetCenter();

		// Revert Looseness, root has no looseness
		const FVector ParentLooseExtent = N.Bounds.GetExtent();
		const FVector ParentTightExtent = (N.Depth == 0) ? ParentLooseExtent : (ParentLooseExtent / Looseness);

		const FVector ChildTightExtent = ParentTightExtent * 0.5f;
		const FVector ChildLooseExtent = ChildTightExtent * Looseness;

		// Initialize all 8 children with loose bounds (octants)
		for (int32 i = 0; i < 8; ++i)
		{
			FVector ChildCenter = ParentCenter;
			ChildCenter.X += ((i & 1) ? 1.f : -1.f) * ChildTightExtent.X;
			ChildCenter.Y += ((i & 2) ? 1.f : -1.f) * ChildTightExtent.Y;
			ChildCenter.Z += ((i & 4) ? 1.f : -1.f) * ChildTightExtent.Z;

			N.Children[i].Bounds = FBox(ChildCenter - ChildLooseExtent, ChildCenter + ChildLooseExtent);
			N.Children[i].Depth = N.Depth + 1;
		}

		// Distribute elements by the center of their bounds
		TArray<TArray<ElementType>> Buckets;
		Buckets.SetNum(8);

		for (const ElementType& E : N.Elements)
		{
			const FBox ElemBounds = OctreeSemantics::GetBoundingBox(E);

			if constexpr (bAllowMultiNode)
			{
				// Insert into ALL child nodes that intersect the bounding box
				for (int32 i = 0; i < 8; ++i)
				{
					if (N.Children[i].Bounds.Intersect(ElemBounds))
					{
						Buckets[i].Add(E);
					}
				}
			}
			else
			{
				// Insert based on center
				const FVector ElemCenter = ElemBounds.GetCenter();
				int32 Index = 0;
				if (ElemCenter.X > ParentCenter.X) Index |= 1;
				if (ElemCenter.Y > ParentCenter.Y) Index |= 2;
				if (ElemCenter.Z > ParentCenter.Z) Index |= 4;
				Buckets[Index].Add(E);
			}
		}

		// Clear elements from this inner node
		N.Elements.Empty();

		for (int32 i = 0; i < 8; ++i)
		{
			if (Buckets[i].Num() > 0)
			{
				N.Children[i].Elements = MoveTemp(Buckets[i]);
				BuildRecursive(N.Children[i]);
			}
			else
			{
				// If a child node is empty, it remains an empty leaf node.
				N.Children[i].Children.Empty(); // Ensure it's a leaf
			}
		}
	}

	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	template<typename TValidator>
	bool TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::Raycast(ElementIdType& OutId, FKzHitResult& OutHit, const FVector& RayStart, const FVector& RayDir, float RayLength, TValidator&& Validator) const
	{
		const float SizeSq = RayDir.SizeSquared();
		if (SizeSq < UE_SMALL_NUMBER)
		{
			UE_LOG(LogTemp, Warning, TEXT("TOctree::Raycast called with zero-length direction"));
			return false;
		}

		FVector Dir = RayDir;
		if (!FMath::IsNearlyEqual(SizeSq, 1.0f))
		{
			Dir *= FMath::InvSqrt(SizeSq);
		}

		if (RayLength <= 0.0f)
		{
			RayLength = UE_BIG_NUMBER;
		}

		OutHit.Init(RayStart, RayStart + Dir * RayLength);
		OutHit.bBlockingHit = false;
		OutHit.Distance = RayLength;

		TSet<ElementIdType> Visited;

		// Begin the recursive traversal starting from the root node.
		RaycastRecursive(Root, OutId, OutHit, RayStart, Dir, RayLength, Forward<TValidator>(Validator), Visited);
		return OutHit.bBlockingHit;
	}

	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	template<typename TValidator>
	void TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::RaycastRecursive(const FNode& N, ElementIdType& OutId, FKzHitResult& OutHit, const FVector& RayStart, const FVector& RayDir, float RayLength, TValidator&& Validator, TSet<ElementIdType>& Visited) const
	{
		// Broad-phase pruning
		const float MaxDist = OutHit.bBlockingHit ? OutHit.Distance : RayLength;
		FKzHitResult BoundsHit;
		if (!Kz::Raycast::Box(BoundsHit, N.Bounds.GetCenter(), N.Bounds.GetExtent(), RayStart, RayDir, MaxDist))
		{
			return;
		}

		if (N.IsLeaf())
		{
			// Narrow phase: test all elements in this leaf node.
			for (const ElementType& E : N.Elements)
			{
				const ElementIdType Id = OctreeSemantics::GetElementId(E);

				// Prevent duplication
				if constexpr (bAllowMultiNode)
				{
					if (Visited.Contains(Id))
					{
						continue;
					}

					Visited.Add(Id);
				}

				if (!OctreeSemantics::IsValid(E) || !Validator(E))
				{
					continue;
				}

				const FKzShapeInstance ElemShape = GetElementShape(E);
				const FVector ElemPos = OctreeSemantics::GetElementPosition(E);
				const FQuat ElemRot = GetElementRotation(E);

				const float MaxCheckLength = OutHit.bBlockingHit ? OutHit.Distance : RayLength;

				const float PrevDist = OutHit.Distance;

				FKzHitResult HitCandidate = OutHit;
				if (Kz::GJK::Raycast(HitCandidate, RayStart, RayDir, MaxCheckLength, ElemShape, ElemPos, ElemRot) && HitCandidate.Distance < PrevDist)
				{
					OutHit = HitCandidate;
					OutId = Id;
				}
			}

			return; // Nothing else below this leaf.
		}

		// --- Internal node: collect children intersected by the ray ---
		struct FChildHit
		{
			const FNode* Node;
			float EntryDist;
		};
		FChildHit Candidates[8];
		int32 NumCandidates = 0;

		for (const FNode& Child : N.Children)
		{
			const float CurrentMaxDist = OutHit.bBlockingHit ? OutHit.Distance : RayLength;
			FKzHitResult ChildHitResult;
			if (Kz::Raycast::Box(ChildHitResult, Child.Bounds.GetCenter(), Child.Bounds.GetExtent(), RayStart, RayDir, CurrentMaxDist))
			{
				Candidates[NumCandidates++] = { &Child, ChildHitResult.Distance };
			}
		}

		if (NumCandidates == 0)
		{
			return; // No children intersected by the ray.
		}

		// Sort by entry distance (ascending)
		Algo::SortBy(MakeArrayView(Candidates, NumCandidates), &FChildHit::EntryDist);

		for (int32 i = 0; i < NumCandidates; ++i)
		{
			const FChildHit& ChildHit = Candidates[i];

			// Early-out: if we already have a closer hit than where this child begins
			if (OutHit.bBlockingHit && ChildHit.EntryDist > OutHit.Distance)
			{
				break;
			}

			RaycastRecursive(*ChildHit.Node, OutId, OutHit, RayStart, RayDir, RayLength, Forward<TValidator>(Validator), Visited);
		}
	}

	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	template<typename TValidator>
	bool TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::Query(TArray<ElementIdType>& OutResults, const FBox& Bounds, TValidator&& Validator) const
	{
		TSet<ElementIdType> Visited;
		QueryRecursive(Root, OutResults, Bounds, Forward<TValidator>(Validator), Visited);
		return !OutResults.IsEmpty();
	}

	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	template<typename TValidator>
	void TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::QueryRecursive(const FNode& N, TArray<ElementIdType>& OutResults, const FBox& Bounds, TValidator&& Validator, TSet<ElementIdType>& Visited) const
	{
		if (!N.Bounds.Intersect(Bounds)) return;

		if (N.IsLeaf())
		{
			for (const ElementType& E : N.Elements)
			{
				const ElementIdType Id = OctreeSemantics::GetElementId(E);

				// Prevent duplication
				if constexpr (bAllowMultiNode)
				{
					if (Visited.Contains(Id))
					{
						continue;
					}

					Visited.Add(Id);
				}

				if (!OctreeSemantics::IsValid(E) || !Validator(E))
				{
					continue;
				}

				if (Bounds.Intersect(OctreeSemantics::GetBoundingBox(E)))
				{
					OutResults.Add(OctreeSemantics::GetElementId(E));
				}
			}
		}
		else
		{
			for (const FNode& Child : N.Children)
			{
				QueryRecursive(Child, OutResults, Bounds, Forward<TValidator>(Validator), Visited);
			}
		}
	}

	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	template<typename TValidator>
	bool TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::Query(TArray<ElementIdType>& OutResults, const FKzShapeInstance& Shape, const FVector& ShapePosition, const FQuat& ShapeRotation, TValidator&& Validator) const
	{
		const FBox QueryAABB = Shape.GetBoundingBox(ShapePosition, ShapeRotation);
		if (!QueryAABB.IsValid)
		{
			return false;
		}

		TSet<ElementIdType> Visited;
		QueryRecursive(Root, OutResults, Shape, ShapePosition, ShapeRotation, QueryAABB, Forward<TValidator>(Validator), Visited);
		return !OutResults.IsEmpty();
	}

	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	template<typename TValidator>
	void TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::QueryRecursive(const FNode& N, TArray<ElementIdType>& OutResults, const FKzShapeInstance& Shape, const FVector& ShapePosition, const FQuat& ShapeRotation, const FBox& QueryAABB, TValidator&& Validator, TSet<ElementIdType>& Visited) const
	{
		// Broad-phase: skip node if its bounds don't intersect the query AABB.
		if (!N.Bounds.Intersect(QueryAABB))
		{
			return;
		}

		if (N.IsLeaf())
		{
			for (const ElementType& E : N.Elements)
			{
				const ElementIdType Id = OctreeSemantics::GetElementId(E);

				// Prevent duplication
				if constexpr (bAllowMultiNode)
				{
					if (Visited.Contains(Id))
					{
						continue;
					}

					Visited.Add(Id);
				}

				if (!OctreeSemantics::IsValid(E) || !Validator(E))
				{
					continue;
				}

				const FKzShapeInstance ElemShape = GetElementShape(E);
				const FVector ElemPos = OctreeSemantics::GetElementPosition(E);
				const FQuat ElemRot = GetElementRotation(E);

				if (Kz::GJK::Intersect(Shape, ShapePosition, ShapeRotation, ElemShape, ElemPos, ElemRot))
				{
					OutResults.Add(OctreeSemantics::GetElementId(E));
				}
			}
		}
		else
		{
			for (const FNode& Child : N.Children)
			{
				QueryRecursive(Child, OutResults, Shape, ShapePosition, ShapeRotation, QueryAABB, Forward<TValidator>(Validator), Visited);
			}
		}
	}

	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	void TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::DebugDraw(const UWorld* World, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness) const
	{
		if (!World)
		{
			return;
		}

		TArray<const FNode*> Stack;
		Stack.Push(&Root);

		while (Stack.Num() > 0)
		{
			const FNode& N = *Stack.Pop(EAllowShrinking::No);

			// Compute extent, compensating for looseness only below the root
			const FVector Extent = N.Bounds.GetExtent() / (N.Depth == 0 ? 1.0f : Looseness);

			// Draw the node AABB
			DrawDebugBox(World, N.Bounds.GetCenter(), Extent, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);

			// Continue traversing children
			for (const FNode& Child : N.Children)
			{
				Stack.Push(&Child);
			}
		}
	}

	// Helpers
	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	FKzShapeInstance TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::GetElementShape(const ElementType& E)
	{
		if constexpr (requires { OctreeSemantics::GetShape(E); })
		{
			// Semantics defines an explicit shape for this element.
			return OctreeSemantics::GetShape(E);
		}
		else
		{
			// Fallback: use bounding sphere derived from bounding box.
			const FBox B = OctreeSemantics::GetBoundingBox(E);
			const float Radius = B.GetExtent().GetAbsMax();
			return FKzShapeInstance::Make<FKzSphere>(Radius);
		}
	}

	template<typename ElementType, typename OctreeSemantics, bool bAllowMultiNode>
	FQuat TOctree<ElementType, OctreeSemantics, bAllowMultiNode>::GetElementRotation(const ElementType& E)
	{
		if constexpr (requires { OctreeSemantics::GetElementRotation(E); })
		{
			return OctreeSemantics::GetElementRotation(E);
		}
		else
		{
			return FQuat::Identity;
		}
	}
}