// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzEcsEntity.h"
#include "KzEcsStorage.h"

namespace Kz::ECS
{
	template<bool bConst, typename IncludedTuple, typename ExcludedTuple>
	class TView;

	template<typename... Components>
	using View = TView<false, TTuple<Components...>, TTuple<>>;

	template<typename... Components>
	using ConstView = TView<true, TTuple<Components...>, TTuple<>>;

	/**
	 * Central ECS registry.
	 *
	 * Responsible for:
	 *   - Creating and destroying entities
	 *   - Managing component storages per type
	 *   - Adding/getting/removing components for entities
	 *   - Maintaining safety via generational handles
	 */
	class Registry
	{
	public:

		// ============================================================
		//  Entity lifetime
		// ============================================================

		/**
		 * Creates a new entity and returns its generational handle.
		 */
		Entity CreateEntity()
		{
			EntityRecord Dummy;
			return Entities.Add(Dummy);
		}

		/**
		 * Destroys an entity and removes all its components.
		 */
		void DestroyEntity(const Entity& E)
		{
			if (!Entities.IsValid(E))
				return;

			// Remove components from all storages
			for (auto& Pair : Storages)
			{
				IStorage* Storage = Pair.Value.Get();
				Storage->Remove(E);
			}

			Entities.Remove(E);
		}

		/**
		 * Checks if the entity is still alive in the registry.
		 */
		bool IsAlive(const Entity& E) const
		{
			return Entities.IsValid(E);
		}

		// ============================================================
		//  Component management
		// ============================================================

		template<typename T>
		T& AddComponent(const Entity& E, const T& Value)
		{
			check(IsAlive(E));
			return GetOrCreateStorage<T>().Add(E, Value);
		}

		template<typename T>
		bool HasComponent(const Entity& E) const
		{
			const Storage<T>* S = GetStorage<T>();
			return S && S->Contains(E);
		}

		template<typename T>
		T& GetComponent(const Entity& E)
		{
			check(IsAlive(E));
			return GetOrCreateStorage<T>().Get(E);
		}

		template<typename T>
		const T& GetComponent(const Entity& E) const
		{
			check(IsAlive(E));
			const Storage<T>* S = GetStorage<T>();
			check(S);
			return S->Get(E);
		}

		template<typename T>
		T* FindComponent(const Entity& E)
		{
			Storage<T>* S = GetStorage<T>();
			return S ? S->Find(E) : nullptr;
		}

		template<typename T>
		const T* FindComponent(const Entity& E) const
		{
			const Storage<T>* S = GetStorage<T>();
			return S ? S->Find(E) : nullptr;
		}

		template<typename T>
		void RemoveComponent(const Entity& E)
		{
			Storage<T>* S = GetStorage<T>();
			if (S) S->Remove(E);
		}

		// ============================================================
		//  Direct storage access (used internally)
		// ============================================================

		template<typename T>
		Storage<T>& GetOrCreateStorage()
		{
			uint32 Id = TypeId<T>();
			TUniquePtr<IStorage>& BasePtr = Storages.FindOrAdd(Id);

			if (!BasePtr.IsValid())
			{
				BasePtr = MakeUnique<Storage<T>>();
			}

			return *static_cast<Storage<T>*>(BasePtr.Get());
		}

		template<typename T>
		Storage<T>* GetStorage()
		{
			uint32 Id = TypeId<T>();
			TUniquePtr<IStorage>* BasePtr = Storages.Find(Id);
			if (!BasePtr || !BasePtr->IsValid())
				return nullptr;

			return static_cast<Storage<T>*>(BasePtr->Get());
		}

		template<typename T>
		const Storage<T>* GetStorage() const
		{
			uint32 Id = TypeId<T>();
			const TUniquePtr<IStorage>* BasePtr = Storages.Find(Id);
			if (!BasePtr || !BasePtr->IsValid())
				return nullptr;

			return static_cast<const Storage<T>*>(BasePtr->Get());
		}

		/**
		 * Creates a mutable view over all entities that contain the specified component types.
		 *
		 * for (auto [e, pos, vel] : Registry.View<FPosition, FVelocity>())
		 * {
		 *     pos.Value += vel.Value * Dt;
		 * }
		 *
		 * @tparam Components  The list of components required on each entity.
		 * @return A View that iterates over entities having all specified components.
		 */
		template<typename... Components>
		View<Components...> View()
		{
			return Kz::ECS::View<Components...>(*this);
		}

		/**
		 * Creates a read-only view over all entities that contain the specified component types.
		 *
		 * const Registry& RConst = Registry;
		 * for (auto [e, pos] : RConst.View<FPosition>())
		 * {
		 *     // pos is const FPosition&
		 * }
		 *
		 * Useful for systems that only read components and can safely run in parallel.
		 *
		 * @tparam Components  The list of components required on each entity.
		 * @return A ConstView that iterates over entities having all specified components.
		 */
		template<typename... Components>
		ConstView<Components...> View() const
		{
			return Kz::ECS::ConstView<Components...>(const_cast<Registry&>(*this));
		}

		template<typename... Components, typename Func>
		void ForEach(Func&& F)
		{
			Kz::ECS::View<Components...>(*this).ForEach(Forward<Func>(F));
		}

		template<typename... Components, typename Func>
		void ParallelForEach(Func&& F)
		{
			Kz::ECS::View<Components...>(*this).ParallelForEach(Forward<Func>(F));
		}

	private:
		EntityPool Entities;
		TMap<uint32, TUniquePtr<IStorage>> Storages;

		/**
		 * Statically-assigned unique type ID for each component T.
		 */
		template<typename T>
		static uint32 TypeId()
		{
			static const uint32 ID = Counter++;
			return ID;
		}

		inline static uint32 Counter = 0;
	};

} // namespace Kz::ECS