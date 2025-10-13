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
#endif

#include "GolfSwing_sensor_metadata.h"
#include "GolfSwing_errors.h"
#include "GolfSwing_authentication.h"

THIRD_PARTY_INCLUDES_END

#include <vector>

// Type aliases
using GSAuthTokenCache = GSAuthTokenCache_t;

haversine::Result<FSuperTagMetadata> FSuperTagExtensions::ParseMetadata(
	const haversine::SatelliteState& State,
	const FString& AuthenticationToken,
	void* Cache)
{
	// Get application data from satellite state
	const std::vector<uint8_t>& AppData = State.persistent().application_data();

	// Deserialize GSSensorMetadata
	haversine::Result<GSSensorMetadata> MetadataResult = DeserializeGSSensorMetadata(
		AppData.data(),
		AppData.size(),
		AuthenticationToken,
		Cache);

	if (!MetadataResult.ok())
	{
		return haversine::Result<FSuperTagMetadata>::error(MetadataResult.status());
	}

	// Convert to SuperTagMetadata
	return haversine::Result<FSuperTagMetadata>::ok(FSuperTagMetadata(MetadataResult.value()));
}

haversine::Result<FSuperTagMetadata> FSuperTagExtensions::ParseMetadata(
	const haversine::SatelliteState& State,
	USuperTagAuthenticationManager* Manager)
{
	if (!Manager)
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagExtensions: Authentication manager is null"));
		return haversine::Result<FSuperTagMetadata>::from_application_code(APPLICATION_ERROR_INVALID_ARGUMENT);
	}

	// Get serial number from state to look up auth token
	std::string SerialNumberStr = State.persistent().serial_number().to_string();
	FString SerialNumber = UTF8_TO_TCHAR(SerialNumberStr.c_str());
	FString AuthToken = Manager->CachedAuthenticationToken(SerialNumber);
	void* CacheHandle = Manager->GetAuthTokenCacheHandle();

	return ParseMetadata(State, AuthToken, CacheHandle);
}

haversine::Result<GSSensorMetadata> FSuperTagExtensions::DeserializeGSSensorMetadata(
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

		// Map GolfSwingKit error to haversine error
		FString ErrorDesc = FString::Printf(TEXT("GolfSwingKit deserialization failed with error code %d"), (int)ErrorCode);
		haversine::Status ErrorStatus("GolfSwingKit", (uint32_t)ErrorCode, TCHAR_TO_UTF8(*ErrorDesc));

		return haversine::Result<GSSensorMetadata>::error(ErrorStatus);
	}

	return haversine::Result<GSSensorMetadata>::ok(Metadata);
}
