using UnrealBuildTool;

public class HorizonClouds : ModuleRules
{
	public HorizonClouds(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bTreatAsEngineModule = true;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"HorizonCloudsCoreShaders",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"RenderCore",
				"Renderer",
				"RHI",
				"Projects",
				"ImGui",
			}
		);
	}
}
