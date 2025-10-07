// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTagGolfSwing.h"
#include "Logging/LogMacros.h"

// Include GolfSwingKit headers
#include "GolfSwingKit.h"
#include "GolfSwing_core.h"
#include "GolfSwing_club.h"
#include "GolfSwing_authentication.h"

DEFINE_LOG_CATEGORY_STATIC(LogSuperTagGolfSwing, Log, All);

FSuperTagGolfSwing::FSuperTagGolfSwing(const std::vector<uint8_t>& CollectionData,
                                       const FString& AuthenticationToken,
                                       GSAuthTokenCache_t* TokenCache)
	: SwingHandle(nullptr)
{
	// Create swing
	SwingHandle = GSCreateSwing();
	if (!SwingHandle)
	{
		UE_LOG(LogSuperTagGolfSwing, Error, TEXT("Failed to create GSSwing_t"));
		return;
	}

	// Convert FString to C string
	const std::string AuthTokenStd = TCHAR_TO_UTF8(*AuthenticationToken);

	// Calculate swing from IMU data
	GSErr Error = GSCalculateSwingFromIMUData(
		SwingHandle,
		CollectionData.data(),
		CollectionData.size(),
		AuthTokenStd.c_str(),
		TokenCache
	);

	if (Error != 0)
	{
		UE_LOG(LogSuperTagGolfSwing, Error, TEXT("Failed to parse swing data: GSErr=%d"), Error);
		GSFreeSwing2(SwingHandle);
		SwingHandle = nullptr;
		return;
	}

	UE_LOG(LogSuperTagGolfSwing, Log, TEXT("Successfully parsed swing data (%d bytes)"), CollectionData.size());
}

FString FSuperTagGolfSwing::ParseHardwareId(const std::vector<uint8_t>& CollectionData)
{
	char HardwareId[13] = {0}; // Hardware ID is 12 characters + null terminator

	GSErr Error = GSGetHardwareIdFromRawData(
		HardwareId,
		CollectionData.data(),
		CollectionData.size()
	);

	if (Error != 0)
	{
		UE_LOG(LogSuperTagGolfSwing, Warning, TEXT("Failed to parse hardware ID from swing data: GSErr=%d"), Error);
		return FString();
	}

	return UTF8_TO_TCHAR(HardwareId);
}

FSuperTagGolfSwing::~FSuperTagGolfSwing()
{
	if (SwingHandle)
	{
		GSFreeSwing2(SwingHandle);
		SwingHandle = nullptr;
	}
}

FSuperTagGolfSwing::FSuperTagGolfSwing(FSuperTagGolfSwing&& Other) noexcept
	: SwingHandle(Other.SwingHandle)
{
	Other.SwingHandle = nullptr;
}

FSuperTagGolfSwing& FSuperTagGolfSwing::operator=(FSuperTagGolfSwing&& Other) noexcept
{
	if (this != &Other)
	{
		// Free existing handle
		if (SwingHandle)
		{
			GSFreeSwing2(SwingHandle);
		}

		// Move from other
		SwingHandle = Other.SwingHandle;
		Other.SwingHandle = nullptr;
	}
	return *this;
}

float FSuperTagGolfSwing::GetClubheadSpeed() const
{
	if (!IsValid())
	{
		return 0.0f;
	}

	return GSGetClubheadSpeedMPH(SwingHandle);
}

FString FSuperTagGolfSwing::GetClub() const
{
	if (!IsValid())
	{
		return TEXT("Unknown");
	}

	GSClub_t Club = GSGetClub(SwingHandle);
	// Club has longName and shortName char arrays
	if (Club.longName[0] != '\0')
	{
		return UTF8_TO_TCHAR(Club.longName);
	}
	else if (Club.shortName[0] != '\0')
	{
		return UTF8_TO_TCHAR(Club.shortName);
	}

	return TEXT("Unknown");
}

FString FSuperTagGolfSwing::GetSensorIdentifier() const
{
	if (!IsValid())
	{
		return TEXT("");
	}

	const char* SensorId = GSGetSensorIdentifier(SwingHandle);
	if (SensorId)
	{
		return UTF8_TO_TCHAR(SensorId);
	}

	return TEXT("");
}

bool FSuperTagGolfSwing::IsRightHanded() const
{
	if (!IsValid())
	{
		return true; // Default to right-handed
	}

	return GSIsRightHanded(SwingHandle) != 0;
}
