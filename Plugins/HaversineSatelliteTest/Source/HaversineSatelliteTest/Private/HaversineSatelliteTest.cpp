// Copyright Epic Games, Inc. All Rights Reserved.

#include "HaversineSatelliteTest.h"

DEFINE_LOG_CATEGORY_STATIC(LogHaversineSatelliteModule, Log, All);

#define LOCTEXT_NAMESPACE "FHaversineSatelliteTestModule"

void FHaversineSatelliteTestModule::StartupModule()
{
	UE_LOG(LogHaversineSatelliteModule, Log, TEXT("HaversineSatelliteTest plugin module loaded"));
	UE_LOG(LogHaversineSatelliteModule, Log, TEXT("Satellite subsystem will start when you press Play in the editor"));
}

void FHaversineSatelliteTestModule::ShutdownModule()
{
	UE_LOG(LogHaversineSatelliteModule, Log, TEXT("HaversineSatelliteTest plugin module unloading"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHaversineSatelliteTestModule, HaversineSatelliteTest)