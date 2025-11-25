// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzEcsRegistry.h"

namespace Kz::ECS
{
	template<bool bConst, typename IncludedTuple, typename ExcludedTuple>
	class TView;

	template<typename... Components>
	using View = TView<false, TTuple<Components...>, TTuple<>>;

	template<typename... Components>
	using ConstView = TView<true, TTuple<Components...>, TTuple<>>;

	/**
	 * Iterates efficiently over all entities that contain ALL specified components.
	 * Automatically selects the smallest storage as the iteration base for performance.
	 *
	 * Example:
	 *   for (auto [e, pos, vel] : registry.View<Position, Velocity>())
	 *   { ... }
	 */
	template<bool bConst, typename... Included, typename... Excluded>
	class TView<bConst, TTuple<Included...>, TTuple<Excluded...>>
	{
	public:
		template<typename T>
		using CompRef = std::conditional_t<bConst, const T&, T&>;

		/** Construct a view bound to a registry instance. */
		TView(Registry& InRegistry)
			: R(InRegistry)
		{
			InitBaseStorage();
		}

		bool IsEntityValid(const Entity& E) const
		{
			if (!((R.HasComponent<Included>(E)) && ...))
				return false;

			if constexpr (sizeof...(Excluded) > 0)
			{
				if (((R.HasComponent<Excluded>(E)) || ...))
					return false;
			}

			return true;
		}

		/**
		 * Returns a new view that excludes entities containing any of the specified components.
		 *
		 * This allows filtering entities based on *absence* of certain components:
		 *
		 * // Iterate over all entities that have FPosition but do NOT have FStatic
		 * for (auto [e, pos] : Registry.View<FPosition>().Exclude<FStatic>())
		 * {
		 *     ...
		 * }
		 *
		 * Multiple Exclude() calls may be chained.
		 *
		 * @tparam MoreExcluded  Additional component types that must NOT be present on an entity.
		 * @return A new TView that includes all previous required components and all new exclusions.
		 */
		template<typename... MoreExcluded>
		auto Exclude() const
		{
			return TView<bConst, TTuple<Included...>, TTuple<Excluded..., MoreExcluded...>>(R);
		}

		/** Calls a lambda for each entity in the view. */
		template<typename Func>
		void ForEach(Func&& F)
		{
			for (auto It = begin(); It != end(); ++It)
			{
				(*It).ApplyAfter(F);
			}
		}

		/** Runs the lambda in parallel for all valid entities. */
		template<typename Func>
		void ParallelForEach(Func&& F)
		{
			if (!Base)
			{
				return;
			}

			const int32 Count = Base->Num();
			if (Count <= 0)
			{
				return;
			}

			ParallelFor(Count, [this, &F](int32 Index)
			{
				Entity E = Base->GetEntities()[Index];

				if (!IsEntityValid(E))
				{
					return;
				}

				F(E, GetComponent<Included>(E)...);
			});
		}

		// ======================================================
		// Iterator definition
		// ======================================================
		struct Iterator
		{
			TView& View;
			IStorage* Base;
			int32 Index;

			Iterator(TView& InView, IStorage* InBase, int32 InIndex)
				: View(InView), Base(InBase), Index(InIndex)
			{
				AdvanceToValid();
			}

			bool operator!=(const Iterator& Other) const
			{
				return Index != Other.Index;
			}

			/**
			 * Dereferences the iterator and returns a tuple:
			 *   (entity, compA&, compB&, ...)
			 */
			auto operator*()
			{
				Entity E = Base->GetEntities()[Index];
				return TTuple<Entity, CompRef<Included>...>(E, View.template GetComponent<Included>(E)...);
			}

			/**
			 * Prefix increment.
			 */
			Iterator& operator++()
			{
				++Index;
				AdvanceToValid();
				return *this;
			}

		private:
			// Check that entity has all components.
			void AdvanceToValid()
			{
				if (!Base)
					return;

				const int32 Count = Base->Num();
				while (Index < Count)
				{
					Entity E = Base->GetEntities()[Index];

					if (View.IsEntityValid(E))
						return;

					++Index;
				}
			}
		};

		Iterator begin()
		{
			return Iterator(*this, Base, 0);
		}

		Iterator end()
		{
			return Iterator(*this, Base, Base->Num());
		}

	private:
		Registry& R;

		// Base storage (the smallest)
		IStorage* Base = nullptr;

		/**
		 * Select the smallest storage to iterate.
		 */
		void InitBaseStorage()
		{
			Base = nullptr;

			// Helper lambda: considers each storage and selects the smallest
			auto Consider = [&](IStorage* S)
			{
				if (S && (!Base || S->Num() < Base->Num()))
				{
					Base = S;
				}
			};

			// Expands over all types of Components...
			(Consider(R.GetStorage<Included>()), ...);

			// If no storage exists, produce an empty view
			if (!Base)
			{
				static FEmptyStorage EmptyStorage;
				Base = &EmptyStorage;
			}
		}

		template<typename T>
		CompRef<T> GetComponent(const Entity& E) const
		{
			return R.GetComponent<T>(E);
		}
	};
} // namespace Kz::ECS