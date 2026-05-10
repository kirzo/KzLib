// Copyright 2026 kirzo

#pragma once

#include <iterator>

/**
 * Concept for types that can be iterated over using range-based for loops.
 * The type must provide valid `begin` and `end` iterators, which are resolvable
 * via std::begin and std::end (this covers both member functions and ADL).
 */
template <typename T>
concept CKzIterable = requires(T && Val)
{
	{ std::begin(Val) };
	{ std::end(Val) };
};