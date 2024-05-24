using System.IO;
using UnrealBuildTool;

public class Simul : ModuleRules
{
	public Simul(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableUndefinedIdentifierWarnings = false;	
		
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"UnrealEd"
				}
			);
		}
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"VRExpansionPlugin", 
			"OpenXRExpansionPlugin", 
			"GameplayTags", 
			"RenderCore",
			"UMG",
			"HeadMountedDisplay",
			"NavigationSystem",
			"PhysicsCore", 
			"MediaAssets"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate", 
			"SlateCore",
			"MoviePlayer",
			"DeveloperSettings",
			"RHI",
			"Niagara"
		});
	}
}
