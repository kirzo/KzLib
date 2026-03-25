// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FKzLibUncookedModule : public IModuleInterface
{
private:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};