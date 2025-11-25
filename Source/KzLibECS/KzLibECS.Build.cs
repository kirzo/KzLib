// Copyright 2025 kirzo

using UnrealBuildTool;

public class KzLibECS : ModuleRules
{
	public KzLibECS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"KzLib"
			}
			);
	}
}