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

	// Mutable API
	requires requires(std::remove_const_t<T>&Mutable)
	{
		// Accept either Empty() or Empty(int32)
			requires (
				requires { Mutable.Empty(); } ||
				requires { Mutable.Empty(std::declval<int32>()); }
				);

	// Accept either Reset() or Reset(int32)
		requires (
			requires { Mutable.Reset(); } ||
			requires { Mutable.Reset(std::declval<int32>()); }
			);
	};
};