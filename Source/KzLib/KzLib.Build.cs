// Copyright 2025 kirzo

using UnrealBuildTool;

public class KzLib : ModuleRules
{
	public KzLib(ReadOnlyTargetRules Target) : base(Target)
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
				"Slate",
				"SlateCore",
				"RenderCore",
				"Projects",
			}
			);

			PublicDefinitions.Add("KZLIB_PLUGIN_NAME=TEXT(\"KzLib\")");
	}
}