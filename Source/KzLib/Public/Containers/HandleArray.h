// Copyright 2025 kirzo

#pragma once

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
template<typename InElementType, CHandleType InHandleType, typename InAllocatorType = FDefaultAllocator>
#else
template<typename InElementType, typename InHandleType, typename InAllocatorType = FDefaultAllocator>
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
			FirstFreeSlot = Slot.NextFreeIndex;
			Slot.NextFreeIndex = INDEX_NONE;
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
		Slot.RealIndex = EntryIndex;

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
		if (!IsValid(Handle))
			return false;
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
		if (!IsValid(Handle))
			return false;

		FSlot& Slot = Slots[Handle.Index];
		ElementType& Element = Entries[Slot.RealIndex].Value;

		Predicate(Element);
		RemoveInternal(Handle);

		return true;
	}

	/** Same as RemoveAfter(), but assumes the handle is valid. */
	template<typename TPredicate>
	void RemoveAfterChecked(const HandleType& Handle, TPredicate Predicate)
	{
		check(IsValid(Handle));
		FSlot& Slot = Slots[Handle.Index];
		ElementType& Element = Entries[Slot.RealIndex].Value;

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
			&& Slots[Handle.Index].Generation == Handle.Generation
			&& Entries.IsValidIndex(Slots[Handle.Index].RealIndex);
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
		if (!IsValid(Handle))
			return nullptr;
		const FSlot& Slot = Slots[Handle.Index];
		return &Entries[Slot.RealIndex].Value;
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
		const FSlot& Slot = Slots[Handle.Index];
		return Entries[Slot.RealIndex].Value;
	}

	/** Const version of FindChecked(). */
	const ElementType& FindChecked(const HandleType& Handle) const
	{
		check(IsValid(Handle));
		const FSlot& Slot = Slots[Handle.Index];
		return Entries[Slot.RealIndex].Value;
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
		const SizeType EntryIndex = SlotToRemove.RealIndex;
		const SizeType LastIndex = Entries.Num() - 1;

		// Maintain dense storage by swapping with the last element if needed
		if (EntryIndex != LastIndex)
		{
			Entries[EntryIndex] = MoveTemp(Entries[LastIndex]);
			FEntry& MovedEntry = Entries[EntryIndex];
			FSlot& MovedSlot = Slots[MovedEntry.SlotIndex];
			MovedSlot.RealIndex = EntryIndex;
		}

		Entries.RemoveAt(LastIndex);

		// Invalidate the removed slot and add it back to the free list
		SlotToRemove.bActive = false;
		++SlotToRemove.Generation;
		SlotToRemove.RealIndex = INDEX_NONE;
		SlotToRemove.NextFreeIndex = FirstFreeSlot;
		FirstFreeSlot = Handle.Index;
	}

	/** Metadata slot describing a stable handle entry. */
	struct FSlot
	{
		int32 Generation = 0;         // Generation counter to detect stale handles.
		SizeType RealIndex = INDEX_NONE; // Actual index into the dense Entries array.
		SizeType NextFreeIndex = INDEX_NONE; // Linked-list pointer for the free slot list.
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

public:
	struct TIterator
	{
		using InternalIter = typename TArray<FEntry>::RangedForIteratorType;

		TIterator(InternalIter Begin, InternalIter End) : EntryIterator(Begin), EndEntryIterator(End) {}

		ElementType& operator*() { FEntry& Entry = *EntryIterator; return Entry.Value; }
		ElementType* operator->() { FEntry& Entry = *EntryIterator; return &Entry.Value; }
		TIterator& operator++() { ++EntryIterator; return *this; }
		explicit operator bool() const { return EntryIterator != EndEntryIterator; }
		friend bool operator==(const TIterator& Lhs, const TIterator& Rhs) { return Lhs.EntryIterator == Rhs.EntryIterator; };
		friend bool operator!=(const TIterator& Lhs, const TIterator& Rhs) { return Lhs.EntryIterator != Rhs.EntryIterator; };

	private:
		InternalIter EntryIterator;
		InternalIter EndEntryIterator;
	};

	struct TConstIterator
	{
		using InternalIter = typename TArray<FEntry>::RangedForConstIteratorType;

		TConstIterator(InternalIter Begin, InternalIter End) : EntryIterator(Begin), EndEntryIterator(End) {}

		const ElementType& operator*() const { const FEntry& Entry = *EntryIterator; return Entry.Value; }
		ElementType const* operator->() const { const FEntry& Entry = *EntryIterator; return &Entry.Value; }
		TConstIterator& operator++() { ++EntryIterator; return *this; }
		explicit operator bool() const { return EntryIterator != EndEntryIterator; }
		friend bool operator==(const TConstIterator& Lhs, const TConstIterator& Rhs) { return Lhs.EntryIterator == Rhs.EntryIterator; };
		friend bool operator!=(const TConstIterator& Lhs, const TConstIterator& Rhs) { return Lhs.EntryIterator != Rhs.EntryIterator; };

	private:
		InternalIter EntryIterator;
		InternalIter EndEntryIterator;
	};

	struct TReverseIterator
	{
		using InternalIter = typename TArray<FEntry>::RangedForReverseIteratorType;

		TReverseIterator(InternalIter Begin, InternalIter End) : EntryIterator(Begin), EndEntryIterator(End) {}

		ElementType& operator*() { FEntry& Entry = *EntryIterator; return Entry.Value; }
		ElementType* operator->() { FEntry& Entry = *EntryIterator; return &Entry.Value; }
		TReverseIterator& operator++() { ++EntryIterator; return *this; }
		explicit operator bool() const { return EntryIterator != EndEntryIterator; }
		friend bool operator==(const TReverseIterator& Lhs, const TReverseIterator& Rhs) { return Lhs.EntryIterator == Rhs.EntryIterator; };
		friend bool operator!=(const TReverseIterator& Lhs, const TReverseIterator& Rhs) { return Lhs.EntryIterator != Rhs.EntryIterator; };

	private:
		InternalIter EntryIterator;
		InternalIter EndEntryIterator;
	};

	struct TConstReverseIterator
	{
		using InternalIter = typename TArray<FEntry>::RangedForConstReverseIteratorType;

		TConstReverseIterator(InternalIter Begin, InternalIter End) : EntryIterator(Begin), EndEntryIterator(End) {}

		const ElementType& operator*() const { const FEntry& Entry = *EntryIterator; return Entry.Value; }
		ElementType const* operator->() const { const FEntry& Entry = *EntryIterator; return &Entry.Value; }
		TConstReverseIterator& operator++() { ++EntryIterator; return *this; }
		explicit operator bool() const { return EntryIterator != EndEntryIterator; }
		friend bool operator==(const TConstReverseIterator& Lhs, const TConstReverseIterator& Rhs) { return Lhs.EntryIterator == Rhs.EntryIterator; };
		friend bool operator!=(const TConstReverseIterator& Lhs, const TConstReverseIterator& Rhs) { return Lhs.EntryIterator != Rhs.EntryIterator; };

	private:
		InternalIter EntryIterator;
		InternalIter EndEntryIterator;
	};

	TIterator CreateIterator()
	{
		return TIterator(Entries.begin(), Entries.end());
	}

	TConstIterator CreateConstIterator() const
	{
		return TConstIterator(Entries.begin(), Entries.end());
	}

	TReverseIterator CreateReverseIterator()
	{
		return TReverseIterator(Entries.rbegin(), Entries.rend());
	}

	TConstReverseIterator CreateConstReverseIterator() const
	{
		return TConstReverseIterator(Entries.rbegin(), Entries.rend());
	}

	// Support to for-range. DO NOT USE DIRECTLY

	FORCEINLINE TIterator      begin() { return TIterator(Entries.begin(), Entries.end()); }
	FORCEINLINE TConstIterator begin() const { return TConstIterator(Entries.begin(), Entries.end()); }
	FORCEINLINE TIterator      end() { return TIterator(Entries.end(), Entries.end()); }
	FORCEINLINE TConstIterator end() const { return TConstIterator(Entries.end(), Entries.end()); }
	FORCEINLINE TReverseIterator      rbegin() { return TReverseIterator(Entries.rbegin(), Entries.rend()); }
	FORCEINLINE TConstReverseIterator rbegin() const { return TConstReverseIterator(Entries.rbegin(), Entries.rend()); }
	FORCEINLINE TReverseIterator      rend() { return TReverseIterator(Entries.rend(), Entries.rend()); }
	FORCEINLINE TConstReverseIterator rend() const { return TConstReverseIterator(Entries.rend(), Entries.rend()); }
};