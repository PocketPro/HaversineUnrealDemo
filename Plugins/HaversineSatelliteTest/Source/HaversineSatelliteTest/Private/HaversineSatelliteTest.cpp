// Copyright Epic Games, Inc. All Rights Reserved.

#include "HaversineSatelliteTest.h"

#define LOCTEXT_NAMESPACE "FHaversineSatelliteTestModule"

void FHaversineSatelliteTestModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogTemp, Warning, TEXT("I AM HERE!!"));
}

void FHaversineSatelliteTestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHaversineSatelliteTestModule, HaversineSatelliteTest)