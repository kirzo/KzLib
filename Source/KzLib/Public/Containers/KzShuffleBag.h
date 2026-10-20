// Copyright 2026 kirzo

#pragma once

#include "Math/RandomStream.h"
#include "Misc/TVariant.h"

namespace Kz
{
	/**
	 * A Shuffle Bag: every entry is drawn once (times its weight) before any repeats, then the bag reshuffles.
	 * An entry can be another bag, in which case drawing it draws from the nested bag. Nested bags keep their own round.
	 */
	template <typename TEntry>
	class TShuffleBag
	{
		using FNested = TSharedRef<TShuffleBag<TEntry>>;
		using FBagEntry = TVariant<TEntry, FNested>;

	public:
		explicit TShuffleBag(int32 Seed = 0) : Rng(Seed != 0 ? Seed : FMath::Rand()) {}

		void Add(const TEntry& Entry, int32 Weight = 1) { AddEntry(FBagEntry(TInPlaceType<TEntry>(), Entry), Weight); }
		void Add(TShuffleBag Bag, int32 Weight = 1) { AddEntry(FBagEntry(TInPlaceType<FNested>(), MakeShared<TShuffleBag>(MoveTemp(Bag))), Weight); }

		TEntry Next()
		{
			checkf(!Pool.IsEmpty(), TEXT("Calling Next() on an empty TShuffleBag!"));

			if (Remaining.IsEmpty())
			{
				Reset();
			}

			const FBagEntry& Entry = Entries[Remaining.Pop()];
			return Entry.template IsType<TEntry>() ? Entry.template Get<TEntry>() : Entry.template Get<FNested>()->Next();
		}

		/** Starts a new round. With bCascade, nested bags start a new round too. */
		void Reset(bool bCascade = false)
		{
			Remaining = Pool;
			for (int32 i = Remaining.Num() - 1; i > 0; --i)
			{
				Remaining.Swap(i, Rng.RandRange(0, i));
			}

			if (bCascade)
			{
				for (FBagEntry& Entry : Entries)
				{
					if (Entry.template IsType<FNested>())
					{
						Entry.template Get<FNested>()->Reset(true);
					}
				}
			}
		}

		void Empty() { Entries.Empty(); Pool.Empty(); Remaining.Empty(); }
		bool IsEmpty() const { return Pool.IsEmpty(); }
		int32 Num() const { return Pool.Num(); }

	private:
		void AddEntry(FBagEntry&& Entry, int32 Weight)
		{
			const int32 Index = Entries.Add(MoveTemp(Entry));
			for (int32 i = 0; i < Weight; ++i)
			{
				Pool.Add(Index);
			}
		}

		TArray<FBagEntry> Entries;
		TArray<int32> Pool;
		TArray<int32> Remaining;
		FRandomStream Rng;
	};
}