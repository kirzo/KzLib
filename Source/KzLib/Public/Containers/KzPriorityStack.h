// Copyright 2026 kirzo

#pragma once

namespace Kz
{
	/**
	 * A Priority Stack (Max-Heap) that allows stable insertion of elements with identical priorities,
	 * optionally keeping the last element and supporting/rejecting duplicate keys.
	 */
	template <typename TEntry, bool bKeepLastElement = false, typename TKey = TEntry, bool bCanContainDuplicates = true>
	class TPriorityStack
	{
	private:
		// Forward declaration of the internal node for the iterators
		struct FPriorityEntry;

	public:
		using TType = TPriorityStack<TEntry, bKeepLastElement, TKey, bCanContainDuplicates>;

		void Empty() { Stack.Empty(); SequenceCounter = 0; }
		bool IsEmpty() const { return Stack.IsEmpty(); }
		int32 Num() const { return Stack.Num(); }

		// --- Top Access ---

		const TEntry& Top() const { return TopEntry(); }
		TEntry& Top() { return TopEntry(); }

		const TEntry& TopEntry() const
		{
			checkf(Num() > 0, TEXT("Calling TopEntry() on an empty TPriorityStack!"));
			return Stack.HeapTop().Entry;
		}

		TEntry& TopEntry()
		{
			checkf(Num() > 0, TEXT("Calling TopEntry() on an empty TPriorityStack!"));
			return Stack.HeapTop().Entry;
		}

		const TKey& TopKey() const
		{
			checkf(Num() > 0, TEXT("Calling TopKey() on an empty TPriorityStack!"));
			return Stack.HeapTop().Key;
		}

		TKey& TopKey()
		{
			checkf(Num() > 0, TEXT("Calling TopKey() on an empty TPriorityStack!"));
			return Stack.HeapTop().Key;
		}

		// --- Push ---

		void Push(const TEntry& Entry, int32 Priority)
		{
			Push(Entry, Entry, Priority);
		}

		void Push(const TEntry& Entry, const TKey& Key, int32 Priority)
		{
			if constexpr (!bCanContainDuplicates)
			{
				if (Contains(Key))
				{
					// Remove existing before pushing new priority
					Stack.RemoveAll([&Key](const FPriorityEntry& E) { return E.Key == Key; });
					Stack.HeapSort();
				}
			}

			// Stable insertion: SequenceCounter guarantees LIFO behavior for identical priorities
			Stack.HeapPush({ Priority, SequenceCounter++, Entry, Key });
		}

		// --- Pop ---

		/** Removes the top element and discards it. Returns true if an element was removed. */
		bool Pop()
		{
			if (IsEmpty() || (bKeepLastElement && Num() == 1))
			{
				return false;
			}

			Stack.HeapPopDiscard();
			return true;
		}

		/** Removes the top element and moves its value to OutEntry. Returns true if successful. */
		bool Pop(TEntry& OutEntry)
		{
			if (IsEmpty() || (bKeepLastElement && Num() == 1))
			{
				return false;
			}

			// Move data safely before discarding the heap node
			OutEntry = MoveTemp(Stack.HeapTop().Entry);
			Stack.HeapPopDiscard();
			return true;
		}

		/** Removes all elements matching the Key. */
		bool Pop(const TKey& Key)
		{
			int32 NumRemoved = 0;

			if constexpr (bKeepLastElement)
			{
				if (Num() <= 1) return false;
			}

			NumRemoved = Stack.RemoveAll([&Key](const FPriorityEntry& E) { return E.Key == Key; });
			if (NumRemoved > 0)
			{
				Stack.HeapSort();
			}

			return NumRemoved > 0;
		}

		// --- Find & Query ---

		TEntry* Find(const TKey& Key)
		{
			FPriorityEntry* Found = Stack.FindByPredicate([&Key](const FPriorityEntry& E) { return E.Key == Key; });
			return Found ? &Found->Entry : nullptr;
		}

		const TEntry* Find(const TKey& Key) const
		{
			const FPriorityEntry* Found = Stack.FindByPredicate([&Key](const FPriorityEntry& E) { return E.Key == Key; });
			return Found ? &Found->Entry : nullptr;
		}

		int32* FindPriority(const TKey& Key)
		{
			FPriorityEntry* Found = Stack.FindByPredicate([&Key](const FPriorityEntry& E) { return E.Key == Key; });
			return Found ? &Found->Priority : nullptr;
		}

		const int32* FindPriority(const TKey& Key) const
		{
			const FPriorityEntry* Found = Stack.FindByPredicate([&Key](const FPriorityEntry& E) { return E.Key == Key; });
			return Found ? &Found->Priority : nullptr;
		}

		bool Contains(const TKey& Key) const
		{
			return Find(Key) != nullptr;
		}

		// --- Utility ---

		template <typename TCallable>
		void ForEach(TCallable Callable)
		{
			for (FPriorityEntry& E : Stack)
			{
				::Invoke(Callable, E.Entry);
			}
		}

		int32 Remove(const TKey& Key)
		{
			int32 Removed = Stack.RemoveAll([&Key](const FPriorityEntry& E) { return E.Key == Key; });
			if (Removed > 0) Stack.HeapSort();
			return Removed;
		}

		template <typename TPredicate>
		int32 RemoveAll(TPredicate Predicate)
		{
			int32 Removed = Stack.RemoveAll([&Predicate](const FPriorityEntry& E) { return ::Invoke(Predicate, E.Entry); });
			if (Removed > 0) Stack.HeapSort();
			return Removed;
		}

		template <typename TPredicate>
		TArray<TEntry> FilterByPredicate(TPredicate Predicate) const
		{
			TArray<TEntry> Result;
			Result.Reserve(Stack.Num()); // Optimize allocation

			for (const FPriorityEntry& E : Stack)
			{
				if (::Invoke(Predicate, E.Entry))
				{
					Result.Add(E.Entry);
				}
			}
			return Result;
		}

	private:

		struct FPriorityEntry
		{
			int32 Priority;
			uint64 Sequence; // Used for stable insertion (LIFO for same priority)
			TEntry Entry;
			TKey Key;

			// Heap compares elements to build a Max-Heap.
			bool operator<(const FPriorityEntry& Other) const
			{
				if (Priority == Other.Priority)
				{
					return Sequence > Other.Sequence;
				}
				return Priority > Other.Priority;
			}
		};

		TArray<FPriorityEntry> Stack;
		uint64 SequenceCounter = 0;

	public:

	// =========================================================================
	//  Iterator Support
	// =========================================================================
	private:
		template<typename TArrayIter>
		struct TBaseIterator
		{
			TBaseIterator(TArrayIter InIter) : Iter(InIter) {}
			auto& operator*() const { return (*Iter).Entry; }
			auto* operator->() const { return &(*Iter).Entry; }
			TBaseIterator& operator++() { ++Iter; return *this; }
			explicit operator bool() const { return (bool)Iter; }
			bool operator!=(const TBaseIterator& Other) const { return Iter != Other.Iter; }
			bool operator==(const TBaseIterator& Other) const { return Iter == Other.Iter; }
		private:
			TArrayIter Iter;
		};

	public:
		using TIterator = TBaseIterator<typename TArray<FPriorityEntry>::RangedForIteratorType>;
		using TConstIterator = TBaseIterator<typename TArray<FPriorityEntry>::RangedForConstIteratorType>;
		using TReverseIterator = TBaseIterator<typename TArray<FPriorityEntry>::RangedForReverseIteratorType>;
		using TConstReverseIterator = TBaseIterator<typename TArray<FPriorityEntry>::RangedForConstReverseIteratorType>;

		TIterator CreateIterator() { return TIterator(Stack.begin()); }
		TConstIterator CreateConstIterator() const { return TConstIterator(Stack.begin()); }
		TReverseIterator CreateReverseIterator() { return TReverseIterator(Stack.rbegin()); }
		TConstReverseIterator CreateConstReverseIterator() const { return TConstReverseIterator(Stack.rbegin()); }

		FORCEINLINE TIterator begin() { return TIterator(Stack.begin()); }
		FORCEINLINE TIterator end() { return TIterator(Stack.end()); }
		FORCEINLINE TConstIterator begin() const { return TConstIterator(Stack.begin()); }
		FORCEINLINE TConstIterator end() const { return TConstIterator(Stack.end()); }
		FORCEINLINE TReverseIterator rbegin() { return TReverseIterator(Stack.rbegin()); }
		FORCEINLINE TReverseIterator rend() { return TReverseIterator(Stack.rend()); }
		FORCEINLINE TConstReverseIterator rbegin() const { return TConstReverseIterator(Stack.rbegin()); }
		FORCEINLINE TConstReverseIterator rend() const { return TConstReverseIterator(Stack.rend()); }
	};
}