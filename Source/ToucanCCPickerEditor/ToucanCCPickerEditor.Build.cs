using UnrealBuildTool;

public class ToucanCCPickerEditor : ModuleRules
{
    public ToucanCCPickerEditor(ReadOnlyTargetRules target) : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "Slate",
                "SlateCore",
                "EditorFramework",
                "LevelEditor",
                "InputCore",
                "Projects",
                "ControlRig",
                "ControlRigEditor",
                "LevelSequence",
                "MovieScene",
                "MovieSceneTools"
            }
        );
    }
}