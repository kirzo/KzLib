// Copyright 2025 kirzo

#pragma once

#include "Core/KzHandle.h" // Default Handle Type

namespace Kz
{
#if __cpp_concepts
// Concept ensuring the handle type follows the required interface
template <typename T>
concept CHandleType = requires(T Handle, int32 Index, int32 Generation)
{
	{ Handle.Index } -> std::convertible_to<int32>;
	{ Handle.Generation } -> std::convertible_to<int32>;
	T(Index, Generation); // must be constructible from (Index, Generation)
};
#else
template <typename T>
struct TIsHandleType
{
private:
	template <typename U>
	static auto Test(int) -> decltype(std::declval<U>().Index, std::declval<U>().Generation, U(int32{}, int32{}), std::true_type{});

	template <typename>
	static auto Test(...) -> std::false_type;

public:
	static constexpr bool Value = decltype(Test<T>(0))::value;
};
#endif

/**
 * Template container that provides stable, generation-based handles to dense array elements.
 *
 * Unlike TArray, elements inside THandleArray can be freely added, removed, or compacted
 * without invalidating existing handles.
 *
 * The container guarantees:
 *  - Dense storage of active elements.
 *  - Safe handle validation and reuse.
 *  - O(1) Add / Remove / Find operations.
 */
#if __cpp_concepts
template<typename InElementType, CHandleType InHandleType = FKzHandle, typename InAllocatorType = FDefaultAllocator>
#else
template<typename InElementType, typename InHandleType = FKzHandle, typename InAllocatorType = FDefaultAllocator>
#endif
class THandleArray
{
#if !__cpp_concepts
	static_assert(TIsHandleType<InHandleType>::Value, "InHandleType must define members 'Index', 'Generation', and be constructible as (int32, int32).");
#endif

public:
	using SizeType = typename InAllocatorType::SizeType;
	using ElementType = InElementType;
	using HandleType = InHandleType;

public:
	/**
	 * Removes all elements and invalidates all existing handles,
	 * but preserves allocated memory for future reuse.
	 */
	void Reset(SizeType NewSize = 0)
	{
		Entries.Reset(NewSize);
		Slots.Reset(NewSize);
		FirstFreeSlot = INDEX_NONE;
	}

	/**
	 * Removes all elements and releases all allocated memory.
	 *
	 * Use this when the container will no longer be used soon,
	 * to minimize memory footprint.
	 */
	void Empty(SizeType Slack = 0)
	{
		Entries.Empty(Slack);
		Slots.Empty(Slack);
		FirstFreeSlot = INDEX_NONE;
	}

	/**
	 * Adds a new element and returns a stable handle referencing it.
	 * Extra arguments are perfectly forwarded to the handle constructor.
	 */
	template<typename... TArgs>
	HandleType Add(const ElementType& Value, TArgs&&... Args)
	{
		static_assert(std::is_constructible_v<HandleType, int32, int32, TArgs...>, "HandleType must be constructible from (int32, int32, Args...)");

		SizeType SlotIndex;

		// Reuse slot if available
		if (FirstFreeSlot != INDEX_NONE)
		{
			SlotIndex = FirstFreeSlot;
			FSlot& Slot = Slots[SlotIndex];
			FirstFreeSlot = Slot.DualIndex;
			Slot.bActive = true;
			++Slot.Generation; // Invalidate old handles
		}
		else
		{
			SlotIndex = Slots.AddDefaulted();
			FSlot& Slot = Slots[SlotIndex];
			Slot.Generation = 1;
			Slot.bActive = true;
		}

		const SizeType EntryIndex = Entries.AddDefaulted();
		FEntry& Entry = Entries[EntryIndex];
		Entry.Value = Value;
		Entry.SlotIndex = SlotIndex;

		// Link slot -> entry
		FSlot& Slot = Slots[SlotIndex];
		Slot.DualIndex = EntryIndex;

		// Construct the handle with forwarded args
		HandleType Handle(SlotIndex, Slot.Generation, Forward<TArgs>(Args)...);

		// If the stored element has a member named "Handle", assign it automatically
		if constexpr (requires(ElementType E) { E.Handle = Handle; })
		{
			Entry.Value.Handle = Handle;
		}

		return Handle;
	}

	/** Removes an element if its handle is valid. */
	bool Remove(const HandleType& Handle)
	{
		if (!IsValid(Handle)) return false;
		RemoveInternal(Handle);
		return true;
	}

	/** Removes the element and assumes the handle is valid. */
	void RemoveChecked(const HandleType& Handle)
	{
		check(IsValid(Handle));
		RemoveInternal(Handle);
	}

	/**
	 * Invokes a user-supplied predicate before removing the element.
	 *
	 * @param Handle     Handle to remove.
	 * @param Predicate  Callable with signature void(ElementType&) or void(const ElementType&).
	 * @return true if the element existed and was removed.
	 */
	template<typename TPredicate>
	bool RemoveAfter(const HandleType& Handle, TPredicate Predicate)
	{
		if (!IsValid(Handle)) return false;

		FSlot& Slot = Slots[Handle.Index];
		ElementType& Element = Entries[Slot.DualIndex].Value;

		::Invoke(Predicate, Element);
		RemoveInternal(Handle);

		return true;
	}

	/** Same as RemoveAfter(), but assumes the handle is valid. */
	template<typename TPredicate>
	void RemoveAfterChecked(const HandleType& Handle, TPredicate Predicate)
	{
		check(IsValid(Handle));
		FSlot& Slot = Slots[Handle.Index];
		ElementType& Element = Entries[Slot.DualIndex].Value;

		::Invoke(Predicate, Element);
		RemoveInternal(Handle);
	}

	/** Returns the number of currently active elements. */
	SizeType Num() const { return Entries.Num(); }

	/**
	 * Returns true if the array is empty and contains no elements.
	 *
	 * @returns True if the pool is empty.
	 * @see Num
	 */
	bool IsEmpty() const { return Entries.IsEmpty(); }

	/**
	 * Tests if index is valid, i.e. greater than or equal to zero, and less than the number of elements in the array.
	 *
	 * @param Index Index to test.
	 * @returns True if index is valid. False otherwise.
	 */
	bool IsValidIndex(SizeType Index) const { return Entries.IsValidIndex(Index); }

	/** Returns true if the provided handle is valid. */
	bool IsValid(const HandleType& Handle) const
	{
		return Slots.IsValidIndex(Handle.Index)
			&& Slots[Handle.Index].bActive
			&& Slots[Handle.Index].Generation == Handle.Generation;
	}

	/** Semantic alias of IsValid() for clarity. */
	bool Contains(const HandleType& Handle) const { return IsValid(Handle); }

	/** Converts a dense index to a handle, forwarding any extra constructor arguments. */
	template<typename... TArgs>
	HandleType IndexToHandle(SizeType Index, TArgs&&... Args) const
	{
		static_assert(std::is_constructible_v<HandleType, int32, int32, TArgs...>, "HandleType must be constructible from (int32, int32, Args...)");

		if (!Entries.IsValidIndex(Index))
		{
			return HandleType(INDEX_NONE, 0);
		}

		const FEntry& Entry = Entries[Index];
		const FSlot& Slot = Slots[Entry.SlotIndex];
		check(Slot.bActive);
		return HandleType(Entry.SlotIndex, Slot.Generation, Forward<TArgs>(Args)...);
	}

	/** Converts a dense index to a handle, forwarding any extra constructor arguments. Assumes index is valid. */
	template<typename... TArgs>
	HandleType IndexToHandleChecked(SizeType Index, TArgs&&... Args) const
	{
		static_assert(std::is_constructible_v<HandleType, int32, int32, TArgs...>, "HandleType must be constructible from (int32, int32, Args...)");

		check(Entries.IsValidIndex(Index));
		const FEntry& Entry = Entries[Index];
		const FSlot& Slot = Slots[Entry.SlotIndex];
		check(Slot.bActive);
		return HandleType(Entry.SlotIndex, Slot.Generation, Forward<TArgs>(Args)...);
	}

	/** Returns a pointer to the element associated with this handle, or nullptr if invalid. */
	ElementType* Find(const HandleType& Handle)
	{
		if (!IsValid(Handle)) return nullptr;
		return &Entries[Slots[Handle.Index].DualIndex].Value;
	}

	/** Const version of Find(). */
	const ElementType* Find(const HandleType& Handle) const
	{
		return const_cast<THandleArray*>(this)->Find(Handle);
	}

	/** Returns a reference to the element associated with this handle. Assumes valid. */
	ElementType& FindChecked(const HandleType& Handle)
	{
		check(IsValid(Handle));
		return Entries[Slots[Handle.Index].DualIndex].Value;
	}

	/** Const version of FindChecked(). */
	const ElementType& FindChecked(const HandleType& Handle) const
	{
		check(IsValid(Handle));
		return Entries[Slots[Handle.Index].DualIndex].Value;
	}

	/** Finds all elements that satisfy the given predicate and appends copies to OutElements. */
	template<typename TPredicate>
	void FindByPredicate(TArray<ElementType>& OutElements, TPredicate Predicate) const
	{
		for (const FEntry& Entry : Entries)
		{
			const ElementType& Element = Entry.Value;
			if (::Invoke(Predicate, Element))
			{
				OutElements.Add(Element);
			}
		}
	}

	/** Finds all handles whose associated elements satisfy the given predicate. */
	template<typename TPredicate>
	void FindHandlesByPredicate(TArray<HandleType>& OutHandles, TPredicate Predicate) const
	{
		for (const FEntry& Entry : Entries)
		{
			const ElementType& Element = Entry.Value;
			if (::Invoke(Predicate, Element))
			{
				const FSlot& Slot = Slots[Entry.SlotIndex];
				if (Slot.bActive)
				{
					OutHandles.Add(HandleType(Entry.SlotIndex, Slot.Generation));
				}
			}
		}
	}

	/**
	 * Direct index operator.
	 *
	 * Accesses elements by dense index (not handle).
	 * This is mainly useful for iteration, debugging, or systems that directly
	 * index into the dense array.
	 */
	ElementType& operator[](SizeType Index)
	{
		check(Entries.IsValidIndex(Index));
		return Entries[Index].Value;
	}

	/** Const version of operator[]. */
	const ElementType& operator[](SizeType Index) const
	{
		check(Entries.IsValidIndex(Index));
		return Entries[Index].Value;
	}

private:
	/** Internal removal logic shared between all Remove* variants. */
	void RemoveInternal(const HandleType& Handle)
	{
		FSlot& SlotToRemove = Slots[Handle.Index];
		const SizeType EntryIndex = SlotToRemove.DualIndex;
		const SizeType LastIndex = Entries.Num() - 1;

		// Maintain dense storage by swapping with the last element if needed
		if (EntryIndex != LastIndex)
		{
			Entries[EntryIndex] = MoveTemp(Entries[LastIndex]);
			FEntry& MovedEntry = Entries[EntryIndex];
			Slots[MovedEntry.SlotIndex].DualIndex = EntryIndex;
		}

		Entries.RemoveAt(LastIndex, EAllowShrinking::No);

		// Invalidate the removed slot and add it back to the free list
		SlotToRemove.bActive = false;
		++SlotToRemove.Generation;
		SlotToRemove.DualIndex = FirstFreeSlot;
		FirstFreeSlot = Handle.Index;
	}

	/** Metadata slot describing a stable handle entry. */
	struct FSlot
	{
		int32 Generation = 0;         // Generation counter to detect stale handles.
		SizeType DualIndex = INDEX_NONE; // DataIndex if Active, NextFree if Inactive
		bool bActive = false;         // Whether this slot currently references a live entry.
	};

	/** Dense array entry storing the actual value and its corresponding slot index. */
	struct FEntry
	{
		ElementType Value;
		SizeType SlotIndex = INDEX_NONE;
	};

	TArray<FEntry, InAllocatorType> Entries;   // Dense array of active elements.
	TArray<FSlot, InAllocatorType>  Slots;     // Indirection table providing handle indirection.
	SizeType FirstFreeSlot = INDEX_NONE; // Head of the free slot linked list.

private:
	// =========================================================================
	//  Iterator Support
	// =========================================================================

	/** Generic Wrapper that adapts TArray Iterators (Forward/Reverse) */
	template<typename TArrayIter>
	struct TBaseIterator
	{
		TBaseIterator(TArrayIter InIter) : Iter(InIter) {}

		// Accessors
		auto& operator*() const { return (*Iter).Value; }
		auto* operator->() const { return &(*Iter).Value; }

		// Advancement
		TBaseIterator& operator++() { ++Iter; return *this; }
		TBaseIterator& operator--() { --Iter; return *this; } // Support bidirectional

		// Comparison
		explicit operator bool() const { return (bool)Iter; }
		bool operator!=(const TBaseIterator& Other) const { return Iter != Other.Iter; }
		bool operator==(const TBaseIterator& Other) const { return Iter == Other.Iter; }

		// Removal support (Delegates to TArray iterator)
		void RemoveCurrent() { Iter.RemoveCurrent(); }

		// Access to internal TArray iterator
		SizeType GetIndex() const { return Iter.GetIndex(); }

	private:
		TArrayIter Iter;
	};

public:
	// Define Iterator Types based on the underlying TArray iterators
	using TIterator = TBaseIterator<typename TArray<FEntry, InAllocatorType>::RangedForIteratorType>;
	using TConstIterator = TBaseIterator<typename TArray<FEntry, InAllocatorType>::RangedForConstIteratorType>;
	using TReverseIterator = TBaseIterator<typename TArray<FEntry, InAllocatorType>::RangedForReverseIteratorType>;
	using TConstReverseIterator = TBaseIterator<typename TArray<FEntry, InAllocatorType>::RangedForConstReverseIteratorType>;

	// --- Factory Methods (Standard Unreal API) ---

	TIterator CreateIterator() { return TIterator(Entries.begin()); }
	TConstIterator CreateConstIterator() const { return TConstIterator(Entries.begin()); }

	TReverseIterator CreateReverseIterator() { return TReverseIterator(Entries.rbegin()); }
	TConstReverseIterator CreateConstReverseIterator() const { return TConstReverseIterator(Entries.rbegin()); }

	// --- STL-style Range Support (For loops) ---

	FORCEINLINE TIterator begin() { return TIterator(Entries.begin()); }
	FORCEINLINE TIterator end() { return TIterator(Entries.end()); }

	FORCEINLINE TConstIterator begin() const { return TConstIterator(Entries.begin()); }
	FORCEINLINE TConstIterator end() const { return TConstIterator(Entries.end()); }

	FORCEINLINE TReverseIterator rbegin() { return TReverseIterator(Entries.rbegin()); }
	FORCEINLINE TReverseIterator rend() { return TReverseIterator(Entries.rend()); }

	FORCEINLINE TConstReverseIterator rbegin() const { return TConstReverseIterator(Entries.rbegin()); }
	FORCEINLINE TConstReverseIterator rend() const { return TConstReverseIterator(Entries.rend()); }
};
}