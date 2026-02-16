// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzHandle.h"
#include "Containers/KzHandleArray.h"

namespace Kz::ECS
{
	/**
	 * Entity identifier for the ECS.
	 *
	 * This is a lightweight generational handle derived from FKzHandle.
	 * It guarantees safe reuse of entity IDs and prevents accidental access
	 * to destroyed entities.
	 */
	struct Entity : public FKzHandle
	{
		using FKzHandle::FKzHandle;
	};

	/**
	 * Per-entity record stored inside the handle pool.
	 */
	struct EntityRecord
	{
		// Reserved for future metadata
	};

	/**
	 * Pool of active entities using.
	 *
	 * This manages:
	 *   - entity creation
	 *   - generational safety
	 *   - recycling of destroyed entity IDs
	 */
	using EntityPool = THandleArray<EntityRecord, Entity>;

} // namespace Kz::ECS