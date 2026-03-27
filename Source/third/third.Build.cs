// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class third : ModuleRules
{
	public third(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 서브디렉토리 간 include 허용 (Game/, UI/ 등)
		PublicIncludePaths.AddRange(new string[] { ModuleDirectory });

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
