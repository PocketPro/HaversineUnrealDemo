// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Configuration parameters for SuperTag system
 * Static configuration for API endpoints and authentication
 */
class SUPERTAGKITPLUGIN_API FSuperTagConfiguration
{
public:
	/** Base URL for SkyGolf API */
	static const FString SkyGolfBaseURL;

	/** Developer key for SkyGolf API authentication */
	static const FString SkygolfDevKey;
};
