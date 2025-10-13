// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTagExtensions.h"
#include "SuperTagAuthenticationManager.h"
#include "SuperKitPlugin.h"
#include "haversine/haversine_satellite_state.h"

// Suppress warnings from third-party GolfSwingKit/PPCommon headers
THIRD_PARTY_INCLUDES_START
#ifdef _MSC_VER
#pragma warning(disable: 4068)  // unknown pragma mark
#pragma warning(disable: 4200)  // zero-sized array in struct
#pragma warning(disable: 4244)  // conversion warnings

// Fix DEPRECATED macro for MSVC (framework headers use GCC __attribute__)
#undef DEPRECATED
#define DEPRECATED(msg) __declspec(deprecated(msg))

// Fix __DEPRECATED_V1__ for MSVC
#undef __DEPRECATED_V1__
#define __DEPRECATED_V1__ __declspec(deprecated("For legacy SkyPro V1 data only"))
#endif

#include "GolfSwing_sensor_metadata.h"
#include "GolfSwing_errors.h"
#include "GolfSwing_authentication.h"

THIRD_PARTY_INCLUDES_END

#include <vector>

// Type aliases
using GSAuthTokenCache = GSAuthTokenCache_t;

FSuperTagMetadata FSuperTagExtensions::ParseMetadata(
	const haversine::SatelliteState& State,
	const FString& AuthenticationToken,
	void* Cache)
{
	// Get application data from satellite state
	const std::vector<uint8_t>& AppData = State.persistent().application_data();

	// Deserialize GSSensorMetadata
	GSSensorMetadata SensorMetadata = DeserializeGSSensorMetadata(
		AppData.data(),
		AppData.size(),
		AuthenticationToken,
		Cache);

	// Convert to SuperTagMetadata
	return FSuperTagMetadata(SensorMetadata);
}

FSuperTagMetadata FSuperTagExtensions::ParseMetadata(
	const haversine::SatelliteState& State,
	USuperTagAuthenticationManager* Manager)
{
	if (!Manager)
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagExtensions: Authentication manager is null"));
		// Return empty metadata
		return FSuperTagMetadata();
	}

	// Get serial number from state to look up auth token
	std::string SerialNumberStr = State.persistent().serial_number().to_string();
	FString SerialNumber = UTF8_TO_TCHAR(SerialNumberStr.c_str());
	FString AuthToken = Manager->CachedAuthenticationToken(SerialNumber);
	void* CacheHandle = Manager->GetAuthTokenCacheHandle();

	return ParseMetadata(State, AuthToken, CacheHandle);
}

GSSensorMetadata FSuperTagExtensions::DeserializeGSSensorMetadata(
	const uint8_t* ApplicationData,
	size_t DataLength,
	const FString& AuthToken,
	void* Cache)
{
	GSSensorMetadata Metadata;

	// Convert FString to C string for GolfSwingKit API
	// Keep the converter alive for the duration of the call
	FTCHARToUTF8 TokenConverter(*AuthToken);
	const char* TokenCStr = AuthToken.IsEmpty() ? nullptr : TokenConverter.Get();

	// Call GolfSwingKit deserialization function
	GSErr ErrorCode = GSSensorMetadata_deserialize(&Metadata, ApplicationData, DataLength, TokenCStr, static_cast<GSAuthTokenCache*>(Cache));

	if (ErrorCode != GSSuccess)
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagExtensions: Failed to deserialize GSSensorMetadata, error code: %d"), (int)ErrorCode);
		// Return empty metadata on error
		GSSensorMetadata EmptyMetadata = {};
		return EmptyMetadata;
	}

	return Metadata;
}
