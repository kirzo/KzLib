// Copyright 2026 kirzo

#pragma once

/** Private namespace to store traits, matching Unreal's pattern. */
namespace NEnumBitmaskRangePrivate
{
	template <typename T>
	struct TEnumBitmaskRangeTraits;
}

/**
 * Range iterator for bitmask enums.
 * Iterates by shifting bits left (Value <<= 1).
 */
template <typename TEnum>
struct TEnumBitmaskRange
{
	struct FIterator
	{
		std::underlying_type_t<TEnum> Value;

		FIterator(std::underlying_type_t<TEnum> InValue) : Value(InValue) {}

		/** Advance to the next bit */
		FIterator& operator++()
		{
			Value <<= 1;
			return *this;
		}

		/** Check if we have reached the end */
		bool operator!=(const FIterator& Other) const
		{
			return Value != Other.Value;
		}

		/** Get the current enum value */
		TEnum operator*() const
		{
			return static_cast<TEnum>(Value);
		}
	};

	FIterator begin() const { return FIterator(NEnumBitmaskRangePrivate::TEnumBitmaskRangeTraits<TEnum>::Begin); }

	FIterator end() const { return FIterator(NEnumBitmaskRangePrivate::TEnumBitmaskRangeTraits<TEnum>::End); }
};

/**
 * Defines a contiguous bitmask enum range with specific start and end values.
 * Iteration proceeds by shifting bits left (First, First << 1, First << 2... up to Last).
 *
 * Example:
 * ENUM_BITMASK_RANGE_BY_FIRST_AND_LAST(EMyBitmask, EMyBitmask::FirstFlag, EMyBitmask::LastFlag)
 */
#define ENUM_BITMASK_RANGE_BY_FIRST_AND_LAST(EnumType, First, Last) \
	namespace NEnumBitmaskRangePrivate \
	{ \
		template <> \
		struct TEnumBitmaskRangeTraits<EnumType> \
		{ \
			static constexpr std::underlying_type_t<EnumType> Begin = static_cast<std::underlying_type_t<EnumType>>(First); \
			static constexpr std::underlying_type_t<EnumType> End   = static_cast<std::underlying_type_t<EnumType>>(Last) << 1; \
		}; \
	}