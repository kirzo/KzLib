// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Algo/Reverse.h"
#include "Containers/KzPriorityStack.h"
#include "UObject/ObjectKey.h"
#include "UObject/WeakObjectPtr.h"

namespace Kz
{
	/** What TBlendLayers keeps for each layer. A layer type derives from it and adds what it carries. */
	struct FBlendLayer
	{
		TWeakObjectPtr<UObject> Source;

		/** Scales the layer, as a volume does by where the target stands in it. */
		float Alpha = 1.0f;

		/** Time blend: 0 to 1 coming in, back to 0 going out. */
		float Blend = 0.0f;
		float BlendTime = 0.0f;

		/** Alpha and blend, minus what newer clearing layers hide. */
		float Weight = 0.0f;

		/** When it was activated, for clearing layers. */
		uint64 Order = 0;

		bool bClearPrevious = false;
		bool bRemoving = false;
	};

	/**
	 * Layers that sources (volumes, markers, Blueprint) add on top of a base, one per source.
	 * Each has a priority (higher on top; among equals, the latest), an alpha and a time blend in and out.
	 * A layer that clears the previous ones hides every layer activated before it, whatever their priority, by its own weight.
	 * A layer goes away when it finishes blending out or its source is destroyed.
	 */
	template <typename TLayer>
	class TBlendLayers
	{
		static_assert(std::is_base_of_v<FBlendLayer, TLayer>, "TBlendLayers needs a layer type derived from Kz::FBlendLayer.");

	public:
		/** Adds Layer for Source, or replaces the one it has; a layer pushed again while blending out blends back in from where it is. */
		void Push(UObject* Source, TLayer Layer, int32 Priority, float BlendTime, bool bClearPrevious, bool bSnap)
		{
			if (!Source) return;

			if (const TLayer* Existing = Layers.Find(Source))
			{
				Layer.Alpha = Existing->Alpha;
				Layer.Blend = Existing->Blend;
			}

			Layer.Source = Source;
			Layer.BlendTime = BlendTime;
			Layer.bClearPrevious = bClearPrevious;
			Layer.bRemoving = false;
			Layer.Order = ++LastOrder;

			if (bSnap)
			{
				Layer.Blend = 1.0f;
			}

			Layers.Push(MoveTemp(Layer), Source, Priority);
		}

		/** Blends out the layer of Source and then removes it. */
		void Pop(const UObject* Source, bool bSnap)
		{
			if (TLayer* Layer = Find(Source))
			{
				Layer->bRemoving = true;
				if (bSnap)
				{
					Layer->Blend = 0.0f;
				}
			}
		}

		void SetAlpha(const UObject* Source, float Alpha)
		{
			if (TLayer* Layer = Find(Source))
			{
				Layer->Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
			}
		}

		TLayer* Find(const UObject* Source) { return Layers.Find(Source); }
		const TLayer* Find(const UObject* Source) const { return Layers.Find(Source); }

		/** Source has a layer that isn't blending out. */
		bool IsActive(const UObject* Source) const
		{
			const TLayer* Layer = Find(Source);
			return Layer && !Layer->bRemoving;
		}

		/** Advances the time blends and drops the layers that finished blending out or lost their source. */
		void Update(float DeltaTime, bool bSnap)
		{
			Layers.ForEach([DeltaTime, bSnap](TLayer& Layer)
			{
				const float Target = Layer.bRemoving ? 0.0f : 1.0f;
				Layer.Blend = bSnap || Layer.BlendTime <= 0.0f ? Target : FMath::FInterpConstantTo(Layer.Blend, Target, DeltaTime, 1.0f / Layer.BlendTime);
			});

			Layers.RemoveAll([](const TLayer& Layer)
			{
				return !Layer.Source.IsValid() || (Layer.bRemoving && Layer.Blend <= 0.0f);
			});
		}

		/** The layers from the lowest priority to the highest, with their weights up to date. */
		TArray<TLayer*> GetOrdered()
		{
			TArray<TLayer*> Ordered = Layers.GetSortedElements();
			Algo::Reverse(Ordered);

			TArray<TLayer*> NewestFirst = Ordered;
			NewestFirst.Sort([](const TLayer& A, const TLayer& B) { return A.Order > B.Order; });

			float Visible = 1.0f;
			for (TLayer* Layer : NewestFirst)
			{
				const float OwnWeight = Layer->Alpha * FMath::SmoothStep(0.0f, 1.0f, Layer->Blend);
				Layer->Weight = OwnWeight * Visible;

				if (Layer->bClearPrevious)
				{
					Visible *= 1.0f - OwnWeight;
				}
			}

			return Ordered;
		}

		template <typename TCallable>
		void ForEach(TCallable Callable)
		{
			Layers.ForEach(Callable);
		}

		bool IsEmpty() const { return Layers.IsEmpty(); }

	private:
		TPriorityStack<TLayer, false, TObjectKey<UObject>, false> Layers;
		uint64 LastOrder = 0;
	};
}