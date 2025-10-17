// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTagMetadata.h"

FSuperTagMetadata::FSuperTagMetadata(const GSSensorMetadata& SensorMetadata)
{
	// Check if the metadata has club information
	if (SensorMetadata.hasClub)
	{
		Club = SensorMetadata.club;
	}
	// else Club remains unset (TOptional)

	// Check if the metadata has a user ID
	if (SensorMetadata.user.userId != 0)
	{
		UserId = SensorMetadata.user.userId;
	}
	// else UserId remains unset (TOptional)

	// Convert the Unix timestamp to FDateTime
	Timestamp = FDateTime::FromUnixTimestamp(SensorMetadata.timestamp);
}

FSuperTagMetadata::FSuperTagMetadata()
	: Timestamp(FDateTime::Now())
{
	// Club and UserId remain unset (TOptional default)
}
