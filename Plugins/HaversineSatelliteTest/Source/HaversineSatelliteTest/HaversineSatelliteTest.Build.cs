// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System.Diagnostics;

public class HaversineSatelliteTest : ModuleRules
{
    public HaversineSatelliteTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // C++20 required by HaversineSatelliteLibrary
        CppStandard = CppStandardVersion.Cpp20;

        // Paths
        string ModulePath = ModuleDirectory;
        string PluginPath = Path.GetFullPath(Path.Combine(ModulePath, "../../"));
        string LibraryPath = Path.Combine(PluginPath, "HaversineSatelliteLibrary_CPP");
        string LibraryBuildPath = Path.Combine(LibraryPath, "build");

        // Build the CMake library
        BuildCMakeLibrary(LibraryPath, LibraryBuildPath, Target);

        // Public headers for your C++ lib (not Apple frameworks)
        PublicIncludePaths.Add(Path.Combine(LibraryPath, "include"));

        PublicDependencyModuleNames.AddRange(new string[] { "Core" });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
        });

        // Link libraries / frameworks per-platform
        LinkLibraries(LibraryPath, LibraryBuildPath, Target);

        // Deploy runtime binaries
        DeployRuntimeBinaries(LibraryPath, Target);
    }

    private void BuildCMakeLibrary(string LibraryPath, string BuildPath, ReadOnlyTargetRules Target)
    {
        string BuildConfig = Target.Configuration == UnrealTargetConfiguration.Debug ? "Debug" : "Release";

        if (!Directory.Exists(BuildPath))
        {
            Directory.CreateDirectory(BuildPath);
        }

        // Configure (idempotent) and build
        string CMakeConfigureArgs = $"-B \"{BuildPath}\" -S \"{LibraryPath}\" -DCMAKE_BUILD_TYPE={BuildConfig} -DHAVERSINE_BUILD_TESTS=OFF -DHAVERSINE_BUILD_EXAMPLES=OFF";
        RunCommand("cmake", CMakeConfigureArgs, LibraryPath);

        string CMakeBuildArgs = $"--build \"{BuildPath}\" --config {BuildConfig}";
        RunCommand("cmake", CMakeBuildArgs, LibraryPath);
    }

    private void LinkLibraries(string LibraryPath, string BuildPath, ReadOnlyTargetRules Target)
    {
        string BuildConfig = Target.Configuration == UnrealTargetConfiguration.Debug ? "Debug" : "Release";

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            //
            // 1) Link your static lib from CMake
            //
            string StaticLibPath = Path.Combine(BuildPath, "libhaversine_satellite.a");
            PublicAdditionalLibraries.Add(StaticLibPath);

            //
            // 2) Link 3rd-party frameworks by giving the linker the *full path* to the binary
            //    inside each .framework. This avoids needing -F framework search paths.
            //
            string FrameworksRoot = Path.Combine(LibraryPath, "frameworks", "macos");

            string HaversineFrameworkDir = Path.Combine(FrameworksRoot, "HaversineUnityPlugin.xcframework", "macos-arm64_x86_64", "HaversineUnityPlugin.framework");
            string PPCommonFrameworkDir   = Path.Combine(FrameworksRoot, "PPCommon.xcframework", "macos-arm64_x86_64", "PPCommon.framework");

            // Linker inputs (point directly at the binary inside the .framework)
            PublicAdditionalLibraries.Add(Path.Combine(HaversineFrameworkDir, "HaversineUnityPlugin"));
            PublicAdditionalLibraries.Add(Path.Combine(PPCommonFrameworkDir, "PPCommon"));

            // Optional (helps IntelliSense/includes if you include headers from the frameworks)
            PublicIncludePaths.Add(Path.Combine(HaversineFrameworkDir, "Headers"));
            PublicIncludePaths.Add(Path.Combine(PPCommonFrameworkDir, "Headers"));

            // NOTE: Do NOT add these names to PublicFrameworks (that emits `-framework` without a path).
            // We already feed absolute paths above, which is more robust.
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // CMake static lib (multi-config on Windows)
            string StaticLibPath = Path.Combine(BuildPath, BuildConfig, "haversine_satellite.lib");
            PublicAdditionalLibraries.Add(StaticLibPath);

            // Vendor import lib + headers
            string WindowsLibPath = Path.Combine(LibraryPath, "frameworks", "windows", "lib");
            PublicAdditionalLibraries.Add(Path.Combine(WindowsLibPath, "HaversineTransportLayer.lib"));

            string WindowsIncludePath = Path.Combine(LibraryPath, "frameworks", "windows", "include");
            PublicIncludePaths.Add(WindowsIncludePath);
        }
    }

    private void DeployRuntimeBinaries(string LibraryPath, ReadOnlyTargetRules Target)
    {
        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            // Stage the full .framework bundles into the app. Unreal will copy directories recursively.
            string FrameworksRoot = Path.Combine(LibraryPath, "frameworks", "macos");
            string HaversineFrameworkDir = Path.Combine(FrameworksRoot, "HaversineUnityPlugin.xcframework", "macos-arm64_x86_64", "HaversineUnityPlugin.framework");
            string PPCommonFrameworkDir   = Path.Combine(FrameworksRoot, "PPCommon.xcframework", "macos-arm64_x86_64", "PPCommon.framework");

            if (Directory.Exists(HaversineFrameworkDir))
            {
                RuntimeDependencies.Add(HaversineFrameworkDir, StagedFileType.NonUFS);
            }
            if (Directory.Exists(PPCommonFrameworkDir))
            {
                RuntimeDependencies.Add(PPCommonFrameworkDir, StagedFileType.NonUFS);
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // Copy Windows DLLs to packaged output
            string WindowsBinPath = Path.Combine(LibraryPath, "frameworks", "windows", "bin");
            string[] DLLs = new string[]
            {
                "HaversineTransportLayer.dll",
                "libopenblas.dll"
            };

            foreach (string dll in DLLs)
            {
                string DLLPath = Path.Combine(WindowsBinPath, dll);
                if (File.Exists(DLLPath))
                {
                    RuntimeDependencies.Add(DLLPath);
                }
            }
        }
    }

    private void RunCommand(string Command, string Arguments, string WorkingDirectory)
    {
        ProcessStartInfo StartInfo = new ProcessStartInfo
        {
            FileName = Command,
            Arguments = Arguments,
            WorkingDirectory = WorkingDirectory,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true
        };

        using (Process Process = new Process { StartInfo = StartInfo })
        {
            Process.Start();
            string Output = Process.StandardOutput.ReadToEnd();
            string Error = Process.StandardError.ReadToEnd();
            Process.WaitForExit();

            if (Process.ExitCode != 0)
            {
                throw new BuildException("Command failed: {0} {1}\n{2}\n{3}", Command, Arguments, Output, Error);
            }
        }
    }
}
