// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Suppress warnings from third-party GolfSwingKit/PPCommon headers
THIRD_PARTY_INCLUDES_START

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundef"
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4068)  // unknown pragma mark
#pragma warning(disable: 4200)  // zero-sized array in struct
#pragma warning(disable: 4244)  // conversion warnings
#endif

// Include GolfSwingKit headers (needed for TOptional to know struct sizes)
#include "GolfSwing_club.h"
#include "GolfSwing_sensor_metadata.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

THIRD_PARTY_INCLUDES_END

// Type aliases for cleaner code
using GSClub = GSClub_t;
using GSSensorMetadata = GSSensorMetadata_t;

/**
 * Satellite metadata container
 * Wraps GolfSwingKit's GSSensorMetadata with convenient Unreal types
 */
struct SUPERKITPLUGIN_API FSuperTagMetadata
{
public:
	/** Associated club (if any) */
	TOptional<GSClub> Club;

	/** Associated user ID (if any) */
	TOptional<uint32> UserId;

	/** The timestamp when this metadata was created */
	FDateTime Timestamp;

	/** Create a new SuperTagMetadata instance from a GSSensorMetadata object */
	explicit FSuperTagMetadata(const GSSensorMetadata& SensorMetadata);

	/** Create a basic metadata instance with no user ID or club information */
	FSuperTagMetadata();
};
