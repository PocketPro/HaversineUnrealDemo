// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperKitPlugin.h"

DEFINE_LOG_CATEGORY_STATIC(LogHaversineSatelliteModule, Log, All);

#define LOCTEXT_NAMESPACE "FSuperKitPluginModule"

void FSuperKitPluginModule::StartupModule()
{
	UE_LOG(LogHaversineSatelliteModule, Log, TEXT("SuperKitPlugin module loaded"));
	UE_LOG(LogHaversineSatelliteModule, Log, TEXT("Satellite subsystem will start when you press Play in the editor"));
}

void FSuperKitPluginModule::ShutdownModule()
{
	UE_LOG(LogHaversineSatelliteModule, Log, TEXT("SuperKitPlugin module unloading"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSuperKitPluginModule, SuperKitPlugin)