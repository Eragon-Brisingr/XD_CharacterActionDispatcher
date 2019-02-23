// Some copyright should be here...

using UnrealBuildTool;

public class XD_CharacterActionDispatcher_Editor : ModuleRules
{
	public XD_CharacterActionDispatcher_Editor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "GraphEditor",
                "BlueprintGraph",
                "KismetCompiler",
                "UnrealEd",
                "EditorStyle",

                "MovieScene",
                "MovieSceneTools",
                "MovieSceneTracks",
                "LevelSequence",
				// ... add private dependencies that you statically link with here ...	

                "XD_CharacterActionDispatcher",
            }
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
