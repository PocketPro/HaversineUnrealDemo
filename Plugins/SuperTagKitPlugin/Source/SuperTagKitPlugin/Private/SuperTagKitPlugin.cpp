// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTagKitPlugin.h"

DEFINE_LOG_CATEGORY_STATIC(LogHaversineSatelliteModule, Log, All);
DEFINE_LOG_CATEGORY(LogHaversineSatellite);

#define LOCTEXT_NAMESPACE "FSuperTagKitPluginModule"

void FSuperTagKitPluginModule::StartupModule()
{
	UE_LOG(LogHaversineSatelliteModule, Log, TEXT("SuperTagKitPlugin module loaded"));
	UE_LOG(LogHaversineSatelliteModule, Log, TEXT("Satellite subsystem will start when you press Play in the editor"));
}

void FSuperTagKitPluginModule::ShutdownModule()
{
	UE_LOG(LogHaversineSatelliteModule, Log, TEXT("SuperTagKitPlugin module unloading"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSuperTagKitPluginModule, SuperTagKitPlugin)