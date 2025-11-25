// Copyright 2025 kirzo

#pragma once

#include "Modules/ModuleManager.h"

class FKzLibECSModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};