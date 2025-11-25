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
			if (int32* Index = Lookup.Find(E))
			{
				T& Ref = Components[*Index];
				Ref = Value;
				return Ref;
			}

			const int32 NewIndex = Components.Add(Value);
			Entities.Add(E);
			Lookup.Add(E, NewIndex);

			return Components[NewIndex];
		}

		/**
		 * Removes the component of the given entity (if present).
		 *
		 * Uses "swap remove" to maintain dense packing.
		 */
		virtual void Remove(const Entity& E) override
		{
			int32* IndexPtr = Lookup.Find(E);
			if (!IndexPtr) return;

			const int32 Index = *IndexPtr;
			const int32 LastIndex = Components.Num() - 1;

			// Move last element into the removed slot
			Components[Index] = Components[LastIndex];
			Entity LastEntity = Entities[LastIndex];

			Entities[Index] = LastEntity;
			Lookup[LastEntity] = Index;

			Components.Pop();
			Entities.Pop();
			Lookup.Remove(E);
		}

		/**
		 * Returns true if the entity owns a component of this type.
		 */
		bool Contains(const Entity& E) const
		{
			return Lookup.Contains(E);
		}

		/**
		 * Direct component access.
		 * Warning: caller must ensure the component exists.
		 */
		T& Get(const Entity& E)
		{
			return Components[Lookup[E]];
		}

		const T& Get(const Entity& E) const
		{
			return Components[Lookup[E]];
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
		TMap<Entity, int32> Lookup;   // Sparse: Entity -> dense index
	};

} // namespace Kz::ECS