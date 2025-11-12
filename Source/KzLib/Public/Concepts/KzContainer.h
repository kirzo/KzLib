// Copyright 2025 kirzo

#pragma once

#include <iterator> // std::begin / std::end

/**
 * Concept for Unreal-style containers that support iteration, size queries, and clearing.
 *
 * The type must provide the following methods:
 *  - `begin` and `end`
 *  - Num(), which returns an int32-like value
 *  - IsEmpty(), which returns a bool
 *  - Empty() (on mutable instances)
 *  - Reset() (on mutable instances)
 */
template <typename T>
concept CKzContainer = requires(T && Val)
{
	// Must be iterable
	{ std::begin(Val) };
	{ std::end(Val) };

	// Must support Num() and IsEmpty() on const
	{ Val.Num() } -> std::convertible_to<int32>;
	{ Val.IsEmpty() } -> std::convertible_to<bool>;

	// Must support Empty() and Reset() when mutable
	requires requires(std::remove_const_t<T>&Mutable)
	{
		{ Mutable.Empty() };
		{ Mutable.Reset() };
	};
};