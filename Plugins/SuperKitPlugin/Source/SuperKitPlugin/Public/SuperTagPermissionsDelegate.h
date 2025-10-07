// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "haversine/haversine_environment.h"

class USuperTagAuthenticationManager;

/**
 * Permission modes for satellite filtering
 */
enum class ESuperTagPermissionMode : uint8
{
	/** Connect and transfer swings from all Haversine satellites */
	AnyUser,

	/** Connect to only satellites that have one of the configured user IDs, or no user ID.
	 *  Only update, transfer swings, etc., from tags that have one of these user IDs (not tags that have no user ID) */
	MultiUser
};

/**
 * Defines what interactions are allowed with which satellites
 * Avoids interactions with satellites belonging to other users, reducing connections,
 * improving latency and battery life, and preventing inadvertent state modifications
 */
class SUPERKITPLUGIN_API FSuperTagPermissionsDelegate : public haversine::HaversinePermissionsDelegate
{
public:
	/**
	 * Constructor
	 *
	 * @param AuthenticationManager The authentication manager to use for token lookups
	 */
	explicit FSuperTagPermissionsDelegate(USuperTagAuthenticationManager* AuthenticationManager);

	virtual ~FSuperTagPermissionsDelegate() = default;

	// HaversinePermissionsDelegate interface
	virtual bool should_handle_advertisement(const HaversineAdvertisement& Advertisement) override;
	virtual bool should_handle_satellite(const haversine::HaversineSatellite& Satellite) override;
	virtual bool should_transfer_collections(const haversine::HaversineSatellite& Satellite) override;

	/**
	 * Sets the permission mode to single user with the specified user ID
	 *
	 * @param UserId The user ID to filter by (0 for no user)
	 */
	void SetSingleUserMode(uint32 UserId);

	/**
	 * Sets the permission mode to multi-user with the specified list of user IDs
	 *
	 * @param UserIds The list of user IDs to filter by
	 */
	void SetMultiUserMode(const TArray<uint32>& UserIds);

	/**
	 * Sets the permission mode to allow any user
	 */
	void SetAnyUserMode();

	/**
	 * Enable or disable swing transfers
	 *
	 * @param bEnabled Whether to allow swing transfers
	 */
	void SetShouldTransferSwings(bool bEnabled);

private:
	/** Current permission mode */
	ESuperTagPermissionMode PermissionMode;

	/** Whether to transfer swings at all */
	bool bShouldTransferSwings;

	/** List of user IDs for MultiUser mode */
	TArray<uint32> UserIdFilter;

	/** Authentication manager for token lookups (weak reference to avoid circular dependency) */
	TWeakObjectPtr<USuperTagAuthenticationManager> AuthenticationManager;

	/**
	 * Helper to parse user ID from satellite state
	 *
	 * @param Satellite The satellite to check
	 * @param OutUserId Output parameter for the user ID
	 * @return True if user ID was successfully parsed, false otherwise
	 */
	bool TryGetUserIdFromSatellite(const haversine::HaversineSatellite& Satellite, uint32& OutUserId) const;
};
