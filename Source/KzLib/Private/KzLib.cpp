// Copyright 2025 kirzo

#include "KzLib.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FKzLibModule"

void FKzLibModule::StartupModule()
{
	// Map Shaders directory.
	const auto Plugin = IPluginManager::Get().FindPlugin(KZLIB_PLUGIN_NAME);
	const FString PluginBaseDir = Plugin.IsValid() ? FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir()) : "";

	const FString PluginShaderDir = FPaths::Combine(PluginBaseDir, TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/") KZLIB_PLUGIN_NAME, PluginShaderDir);
}

void FKzLibModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FKzLibModule, KzLib)