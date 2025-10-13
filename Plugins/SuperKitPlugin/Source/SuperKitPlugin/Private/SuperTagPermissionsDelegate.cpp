// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTagPermissionsDelegate.h"
#include "SuperTagAuthenticationManager.h"
#include "SuperTagExtensions.h"
#include "haversine/haversine_satellite.h"
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

THIRD_PARTY_INCLUDES_END

#include "SuperKitPlugin.h"

FSuperTagPermissionsDelegate::FSuperTagPermissionsDelegate(USuperTagAuthenticationManager* InAuthenticationManager)
	: PermissionMode(ESuperTagPermissionMode::AnyUser)
	, bShouldTransferSwings(true)
	, AuthenticationManager(InAuthenticationManager)
{
}

bool FSuperTagPermissionsDelegate::should_handle_advertisement(const HaversineAdvertisement& Advertisement)
{
	switch (PermissionMode)
	{
	case ESuperTagPermissionMode::AnyUser:
		return true;

	case ESuperTagPermissionMode::MultiUser:
		{
			uint32 Fingerprint = Advertisement.manufacturerData.persistentStateFingerprint;

			// Check if fingerprint might match one of our user IDs
			for (uint32 UserId : UserIdFilter)
			{
				if (GSSensorMetadata_fingerprintMatchesUserId(Fingerprint, UserId))
				{
					return true;
				}
			}

			// Also accept satellites with no user ID
			if (GSSensorMetadata_fingerprintMatchesNoUser(Fingerprint))
			{
				return true;
			}

			return false;
		}

	default:
		return true;
	}
}

bool FSuperTagPermissionsDelegate::should_handle_satellite(const haversine::HaversineSatellite& Satellite)
{
	// Always handle satellites in fail-safe mode (maritime law!)
	if (Satellite.state().is_in_fail_safe_mode())
	{
		return true;
	}

	// Attempt to update authentication based on hardware ID (serial number)
	if (AuthenticationManager.IsValid())
	{
		FString HardwareId = UTF8_TO_TCHAR(Satellite.state().persistent().serial_number().to_string().c_str());
		AuthenticationManager->UpdateAuthenticationIfNecessary(HardwareId);

		// Check if we have a valid authentication token
		if (!AuthenticationManager->HasCachedAuthenticationToken(HardwareId))
		{
			return false;
		}
	}

	switch (PermissionMode)
	{
	case ESuperTagPermissionMode::AnyUser:
		return true;

	case ESuperTagPermissionMode::MultiUser:
		{
			uint32 UserId = 0;
			if (TryGetUserIdFromSatellite(Satellite, UserId))
			{
				return UserIdFilter.Contains(UserId);
			}
			return false;
		}

	default:
		return true;
	}
}

bool FSuperTagPermissionsDelegate::should_transfer_collections(const haversine::HaversineSatellite& Satellite)
{
	if (!bShouldTransferSwings)
	{
		return false;
	}

	// Check for authentication token
	if (AuthenticationManager.IsValid())
	{
		FString HardwareId = UTF8_TO_TCHAR(Satellite.state().persistent().serial_number().to_string().c_str());
		if (!AuthenticationManager->HasCachedAuthenticationToken(HardwareId))
		{
			return false;
		}
	}

	switch (PermissionMode)
	{
	case ESuperTagPermissionMode::AnyUser:
		return true;

	case ESuperTagPermissionMode::MultiUser:
		{
			uint32 UserId = 0;
			if (TryGetUserIdFromSatellite(Satellite, UserId))
			{
				return UserIdFilter.Contains(UserId);
			}
			return false;
		}

	default:
		return true;
	}
}

void FSuperTagPermissionsDelegate::SetSingleUserMode(uint32 UserId)
{
	PermissionMode = ESuperTagPermissionMode::MultiUser;
	UserIdFilter.Empty();
	if (UserId != 0)
	{
		UserIdFilter.Add(UserId);
	}
}

void FSuperTagPermissionsDelegate::SetMultiUserMode(const TArray<uint32>& UserIds)
{
	PermissionMode = ESuperTagPermissionMode::MultiUser;
	UserIdFilter = UserIds;
}

void FSuperTagPermissionsDelegate::SetAnyUserMode()
{
	PermissionMode = ESuperTagPermissionMode::AnyUser;
	UserIdFilter.Empty();
}

void FSuperTagPermissionsDelegate::SetShouldTransferSwings(bool bEnabled)
{
	bShouldTransferSwings = bEnabled;
}

bool FSuperTagPermissionsDelegate::TryGetUserIdFromSatellite(const haversine::HaversineSatellite& Satellite, uint32& OutUserId) const
{
	if (!AuthenticationManager.IsValid())
	{
		return false;
	}

	try
	{
		// Parse metadata with authentication token
		FSuperTagMetadata Metadata = FSuperTagExtensions::ParseMetadata(Satellite.state(), AuthenticationManager.Get());

		if (Metadata.UserId.IsSet())
		{
			OutUserId = Metadata.UserId.GetValue();
			return true;
		}
	}
	catch (...)
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagPermissionsDelegate: Error parsing metadata from satellite"));
	}

	return false;
}
