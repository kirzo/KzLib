// Copyright 2026 kirzo

#include "Kismet/KzAppLibrary.h"
#include "Misc/ConfigCacheIni.h"

FString UKzAppLibrary::GetProjectVersion()
{
	// Declare the string variable to hold the output
	FString ProjectVersion;

	// Read the ProjectVersion directly from the standard Game.ini file
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GeneralProjectSettings"),
		TEXT("ProjectVersion"),
		ProjectVersion,
		GGameIni
	);

	return ProjectVersion;
}