// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzEcsEntity.h"

namespace Kz::ECS
{
	/**
	 * Base class for component storage.
	 *
	 * The ECS registry stores pointers to this interface so it can call
	 * Remove(Entity) generically for any component type when an entity is destroyed.
	 */
	struct IStorage
	{
		virtual ~IStorage() = default;
		virtual void Remove(const Entity& E) = 0;
		virtual int32 Num() const = 0;
		virtual const TArray<Entity>& GetEntities() const = 0;
	};

	struct FEmptyStorage : IStorage
	{
		FEmptyStorage() = default;
		virtual void Remove(const Entity&) override {}
		virtual int32 Num() const override { return 0; }
		virtual const TArray<Entity>& GetEntities() const override
		{
			static TArray<Entity> Empty;
			return Empty;
		}
	};

	/**
	 * Sparse-set component storage for a specific component type T.
	 *
	 * Layout:
	 *   Dense:
	 *     - Components[] : tightly-packed array of component values
	 *     - Entities[]   : parallel array of owning entities
	 *   Sparse:
	 *     - Lookup map: Entity -> dense index
	 *
	 * Dense arrays guarantee cache-friendly iteration.
	 * Lookup map enables O(1) access for Get/Contains.
	 */
	template<typename T>
	class Storage : public IStorage
	{
	public:
		/**
		 * Adds or overwrites the component for the given entity.
		 */
		T& Add(const Entity& E, const T& Value)
		{
			EnsureCapacity(E.Index);
			
			const int32 DenseIndex = Sparse[E.Index];
			if (DenseIndex != INDEX_NONE && DenseIndex < Components.Num())
			{
				// Already exists, overwrite
				T& Ref = Components[DenseIndex];
				Ref = Value;
				return Ref;
			}

			// New component
			const int32 NewIndex = Components.Add(Value);
			Entities.Add(E);
			Sparse[E.Index] = NewIndex;

			return Components[NewIndex];
		}

		/**
		 * Removes the component of the given entity (if present).
		 *
		 * Uses "swap remove" to maintain dense packing.
		 */
		virtual void Remove(const Entity& E) override
		{
			if (!Contains(E)) return;

			const int32 Index = Sparse[E.Index];
			const int32 LastIndex = Components.Num() - 1;

			// If we are removing the last element, no swap needed (just pop)
			if (Index != LastIndex)
			{
				// Move last element into the removed slot
				Components[Index] = Components[LastIndex];
				Entity LastEntity = Entities[LastIndex];

				Entities[Index] = LastEntity;
				
				// Update sparse array for the moved entity
				// Using E.Index for the entity we are removing is correct, 
				// but we need to update the entry for LastEntity.
				Sparse[LastEntity.Index] = Index;
			}

			Components.Pop();
			Entities.Pop();

			// Mark slot as empty
			Sparse[E.Index] = INDEX_NONE;
		}

		/**
		 * Returns true if the entity owns a component of this type.
		 */
		bool Contains(const Entity& E) const
		{
			return E.Index >= 0 && E.Index < Sparse.Num() && Sparse[E.Index] != INDEX_NONE;
		}

		/**
		 * Direct component access.
		 * Warning: caller must ensure the component exists.
		 */
		T& Get(const Entity& E)
		{
			// Assumes Contains(E) is true or checked by caller (Registry checks IsAlive)
			return Components[Sparse[E.Index]];
		}

		const T& Get(const Entity& E) const
		{
			return Components[Sparse[E.Index]];
		}

		T* Find(const Entity& E)
		{
			return Contains(E) ? &Components[Sparse[E.Index]] : nullptr;
		}

		const T* Find(const Entity& E) const
		{
			return Contains(E) ? &Components[Sparse[E.Index]] : nullptr;
		}

		/**
		 * Number of components stored.
		 */
		int32 Num() const
		{
			return Components.Num();
		}

		/**
		 * Dense arrays for iteration (used internally by views/systems).
		 */
		const TArray<T>& GetComponents() const { return Components; }
		const TArray<Entity>& GetEntities() const { return Entities; }

	private:
		TArray<T> Components;         // Dense component array
		TArray<Entity> Entities;      // Dense owner list
		TArray<int32> Sparse;         // Sparse: Entity Index -> Dense Index

		void EnsureCapacity(int32 EntityIndex)
		{
			if (EntityIndex >= Sparse.Num())
			{
				const int32 OldNum = Sparse.Num();
				const int32 NewNum = EntityIndex + 1;
				Sparse.SetNum(NewNum);
				
				// Initialize new slots to INDEX_NONE
				for (int32 i = OldNum; i < NewNum; ++i)
				{
					Sparse[i] = INDEX_NONE;
				}
			}
		}
	};

} // namespace Kz::ECS