// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SuperTagMetadata.h"
#include "haversine/result.h"

// Forward declarations
namespace haversine
{
	class SatelliteState;
}
class USuperTagAuthenticationManager;

/**
 * Extension utilities for parsing satellite state metadata
 * Provides convenient methods for extracting SuperTag metadata from Haversine satellite states
 */
class SUPERKITPLUGIN_API FSuperTagExtensions
{
public:
	/**
	 * Parses the application data from the satellite state using the provided authentication token
	 *
	 * @param State The satellite state to parse
	 * @param AuthenticationToken An optional authentication token for decrypting metadata
	 * @param Cache An optional cache handle for speeding up authentication
	 * @return Result containing the parsed metadata on success, or error on failure
	 */
	static haversine::Result<FSuperTagMetadata> ParseMetadata(
		const haversine::SatelliteState& State,
		const FString& AuthenticationToken = FString(),
		void* Cache = nullptr);

	/**
	 * Parses the application data from the satellite state using the authentication manager
	 * This is a convenience overload that automatically retrieves the token and cache
	 *
	 * @param State The satellite state to parse
	 * @param Manager The authentication manager to use for token lookup
	 * @return Result containing the parsed metadata on success, or error on failure
	 */
	static haversine::Result<FSuperTagMetadata> ParseMetadata(
		const haversine::SatelliteState& State,
		USuperTagAuthenticationManager* Manager);

private:
	/**
	 * Internal helper to deserialize GSSensorMetadata from raw application data
	 *
	 * @param ApplicationData Byte array containing the application data
	 * @param DataLength Length of the application data
	 * @param AuthToken Authentication token (optional)
	 * @param Cache Cache handle (optional)
	 * @return Result containing deserialized GSSensorMetadata on success, or error on failure
	 */
	static haversine::Result<GSSensorMetadata> DeserializeGSSensorMetadata(
		const uint8_t* ApplicationData,
		size_t DataLength,
		const FString& AuthToken,
		void* Cache);
};
