// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

SUPERKITPLUGIN_API DECLARE_LOG_CATEGORY_EXTERN(LogHaversineSatellite, Log, All);

class FSuperKitPluginModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
