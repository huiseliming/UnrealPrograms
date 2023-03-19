// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ProgramPackageTool : ModuleRules
{
	public ProgramPackageTool(ReadOnlyTargetRules Target) : base(Target)
	{
		bEnforceIWYU = true;
		bLegacyPublicIncludePaths = false;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.Combine(EngineDirectory, "Source", "Runtime/Launch/Public"));

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"AppFramework",
				"Core",
				"ApplicationCore",
				"Projects",
				"Slate",
				"SlateCore",
				"InputCore",
				"CoreUObject",
				"StandaloneRenderer",
				"Json",
                "DesktopPlatform",
            }
		);

        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory));		// For LaunchEngineLoop.cpp include
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source", "Runtime/Launch/Private"));		// For LaunchEngineLoop.cpp include

		if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.TVOS)
		{
			PrivateDependencyModuleNames.AddRange(
                new string [] {
                    "NetworkFile",
                    "StreamingFile"
                }
            );
		}

		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Linux))
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"UnixCommonStartup"
				}
			);
		}
	}
}
