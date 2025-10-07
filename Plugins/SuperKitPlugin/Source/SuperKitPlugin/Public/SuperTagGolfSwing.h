// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Forward declarations (GolfSwingKit is C, avoid including in header)
struct GSSwing_t_;
typedef struct GSSwing_t_ GSSwing_t;
struct GSAuthTokenCache_s;
typedef struct GSAuthTokenCache_s GSAuthTokenCache_t;

/**
 * RAII wrapper around GolfSwingKit's GSSwing_t
 * Provides safe construction/destruction and exposes raw handle for direct GolfSwingKit API access
 *
 * Usage:
 *   FSuperTagGolfSwing swing(collectionData, authToken, tokenCache);
 *
 *   // Option 1: Use convenience methods
 *   float speed = swing.GetClubheadSpeed();
 *
 *   // Option 2: Direct GolfSwingKit C API access
 *   GSGetParameterForKey(swing.GetHandle(), kSomeParameter);
 */
class SUPERKITPLUGIN_API FSuperTagGolfSwing
{
public:
	/**
	 * Create and parse a golf swing from raw collection data
	 * @param CollectionData Raw swing data from satellite
	 * @param AuthenticationToken Token for authenticating the swing data
	 * @param TokenCache Optional cache for authentication tokens (can be nullptr)
	 * @throws Will log error and create invalid swing if parsing fails
	 */
	FSuperTagGolfSwing(const std::vector<uint8_t>& CollectionData,
	                   const FString& AuthenticationToken,
	                   GSAuthTokenCache_t* TokenCache);

	/**
	 * Parse hardware ID from raw collection data without creating a full swing
	 * @param CollectionData Raw swing data from satellite
	 * @return Hardware ID string, or empty string on error
	 */
	static FString ParseHardwareId(const std::vector<uint8_t>& CollectionData);

	// Destructor - automatically frees the GSSwing_t handle
	~FSuperTagGolfSwing();

	// No copy (GSSwing_t is unique)
	FSuperTagGolfSwing(const FSuperTagGolfSwing&) = delete;
	FSuperTagGolfSwing& operator=(const FSuperTagGolfSwing&) = delete;

	// Move semantics
	FSuperTagGolfSwing(FSuperTagGolfSwing&& Other) noexcept;
	FSuperTagGolfSwing& operator=(FSuperTagGolfSwing&& Other) noexcept;

	/**
	 * Get the raw GSSwing_t handle for direct GolfSwingKit C API access
	 * Allows calling any GSGetXXX() function directly
	 * @return Raw handle, may be nullptr if construction failed
	 */
	GSSwing_t* GetHandle() const { return SwingHandle; }

	/**
	 * Check if this swing is valid (construction succeeded)
	 */
	bool IsValid() const { return SwingHandle != nullptr; }

	//
	// Convenience methods (wrapper around common GolfSwingKit functions)
	// Users can also call GolfSwingKit directly via GetHandle()
	//

	/**
	 * Get clubhead speed in MPH
	 * @return Speed in MPH, or 0.0 if invalid
	 */
	float GetClubheadSpeed() const;

	/**
	 * Get the club type for this swing
	 * @return Club identifier string
	 */
	FString GetClub() const;

	/**
	 * Get the sensor identifier (hardware ID)
	 * @return Sensor identifier string
	 */
	FString GetSensorIdentifier() const;

	/**
	 * Check if this is a right-handed swing
	 * @return true if right-handed, false if left-handed
	 */
	bool IsRightHanded() const;

private:
	GSSwing_t* SwingHandle;
};
