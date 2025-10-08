// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTagGolfSwing.h"
#include "Logging/LogMacros.h"

// Include GolfSwingKit headers
#include "GolfSwingKit.h"
#include "GolfSwing_core.h"
#include "GolfSwing_club.h"
#include "GolfSwing_authentication.h"
#include "GolfSwing_types.h"

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

bool FSuperTagGolfSwing::IsAirswing() const
{
	if (!IsValid())
	{
		return false;
	}

	return GSSwingIsAirswing(SwingHandle) != 0;
}

// Swing Timing Methods

int32 FSuperTagGolfSwing::GetTimeSwingStart() const
{
	if (!IsValid())
	{
		return -1;
	}

	return GSTimeSwingStart(SwingHandle);
}

int32 FSuperTagGolfSwing::GetTimeMidBackswing() const
{
	if (!IsValid())
	{
		return -1;
	}

	return GSTimeMidBackswing(SwingHandle);
}

int32 FSuperTagGolfSwing::GetTimeTopOfBackswing() const
{
	if (!IsValid())
	{
		return -1;
	}

	return GSTimeTopOfBackswing(SwingHandle);
}

int32 FSuperTagGolfSwing::GetTimeMidDownswing() const
{
	if (!IsValid())
	{
		return -1;
	}

	return GSTimeMidDownswing(SwingHandle);
}

int32 FSuperTagGolfSwing::GetTimeImpact() const
{
	if (!IsValid())
	{
		return -1;
	}

	return GSTimeImpact(SwingHandle);
}

int32 FSuperTagGolfSwing::GetTimeSwingEnd() const
{
	if (!IsValid())
	{
		return -1;
	}

	return GSTimeSwingEnd(SwingHandle);
}

uint32 FSuperTagGolfSwing::GetSwingTimestamp() const
{
	if (!IsValid())
	{
		return 0;
	}

	uint32 Timestamp = 0;
	GSErr Error = GSGetSwingTimestampInSeconds(SwingHandle, &Timestamp);
	if (Error != 0)
	{
		return 0;
	}

	return Timestamp;
}

// Swing Parameters

bool FSuperTagGolfSwing::GetParameterForKey(int32 Key, float& OutValue, int32& OutStatus) const
{
	if (!IsValid())
	{
		OutValue = 0.0f;
		OutStatus = -1;
		return false;
	}

	GSParameter_t Parameter = GSGetParameterForKey(SwingHandle, static_cast<GSParameterKey_t>(Key));
	OutValue = Parameter.value;
	OutStatus = Parameter.status;

	return Parameter.status == 0;
}


// Position and Orientation Methods

int32 FSuperTagGolfSwing::GetSceneOriginLab(FVector& OutOrigin) const
{
	if (!IsValid())
	{
		OutOrigin = FVector::ZeroVector;
		return -1;
	}

	GSVectorElement_t Origin[3] = {0, 0, 0};
	GSErr Error = GSGetSceneOriginLab(SwingHandle, Origin);

	// Convert to Unreal's FVector
	OutOrigin = FVector(Origin[0], Origin[1], Origin[2]);

	return Error;
}

int32 FSuperTagGolfSwing::GetPositionLab(FVector& OutPosition, int32 PointLocation, int32 Timestamp) const
{
	if (!IsValid())
	{
		OutPosition = FVector::ZeroVector;
		return -1;
	}

	GSVectorElement_t Position[3] = {0, 0, 0};

	// Create club point location from landmark
	GSClubPointLocation_t Point = GSMakeClubPointLocation(static_cast<GSClubLandmark_t>(PointLocation));

	GSErr Error = GSGetPositionLab(SwingHandle, Position, Point, Timestamp);

	// Convert to Unreal's FVector
	OutPosition = FVector(Position[0], Position[1], Position[2]);

	return Error;
}

int32 FSuperTagGolfSwing::GetPositionScene(FVector& OutPosition, int32 PointLocation, int32 Timestamp) const
{
	if (!IsValid())
	{
		OutPosition = FVector::ZeroVector;
		return -1;
	}

	GSVectorElement_t Position[3] = {0, 0, 0};

	// Create club point location from landmark
	GSClubPointLocation_t Point = GSMakeClubPointLocation(static_cast<GSClubLandmark_t>(PointLocation));

	GSErr Error = GSGetPositionScene(SwingHandle, Position, Point, Timestamp);

	// Convert to Unreal's FVector
	OutPosition = FVector(Position[0], Position[1], Position[2]);

	return Error;
}

int32 FSuperTagGolfSwing::GetClubheadPathDistance(float& OutDistance, int32 Timestamp) const
{
	if (!IsValid())
	{
		OutDistance = 0.0f;
		return -1;
	}

	GSVectorElement_t Distance = 0.0;
	GSErr Error = GSGetClubheadPathDistance(SwingHandle, &Distance, Timestamp);

	OutDistance = static_cast<float>(Distance);

	return Error;
}

int32 FSuperTagGolfSwing::GetTimeForClubHeadPathDistance(int32& OutTimestamp, float PathDistance) const
{
	if (!IsValid())
	{
		OutTimestamp = -1;
		return -1;
	}

	GSTimestamp_t Timestamp = 0;
	GSErr Error = GSGetTimeForClubHeadPathDistance(SwingHandle, &Timestamp, PathDistance);

	OutTimestamp = Timestamp;

	return Error;
}

// Coordinate Transformation Methods

int32 FSuperTagGolfSwing::GetBodyToLabMatrix(TArray<double>& OutMatrix, int32 Timestamp, int32 PointLocation) const
{
	if (!IsValid())
	{
		OutMatrix.SetNum(9);
		for (int32 i = 0; i < 9; i++)
		{
			OutMatrix[i] = 0.0;
		}
		return -1;
	}

	GSVectorElement_t Matrix[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

	// Create club point location from landmark
	GSClubPointLocation_t Point = GSMakeClubPointLocation(static_cast<GSClubLandmark_t>(PointLocation));

	GSErr Error = GSGetBodyToLabMatrix(SwingHandle, Matrix, Timestamp, Point);

	// Copy to output array
	OutMatrix.SetNum(9);
	for (int32 i = 0; i < 9; i++)
	{
		OutMatrix[i] = Matrix[i];
	}

	return Error;
}

int32 FSuperTagGolfSwing::TransformFromBodyToLab(FVector& OutLabVector, const FVector& BodyVector, int32 Timestamp, int32 PointLocation) const
{
	if (!IsValid())
	{
		OutLabVector = FVector::ZeroVector;
		return -1;
	}

	GSVectorElement_t LabVec[3] = {0, 0, 0};
	GSVectorElement_t BodyVec[3] = {BodyVector.X, BodyVector.Y, BodyVector.Z};

	// Create club point location from landmark
	GSClubPointLocation_t Point = GSMakeClubPointLocation(static_cast<GSClubLandmark_t>(PointLocation));

	GSErr Error = GSTransformFromBodyToLab(SwingHandle, LabVec, BodyVec, Timestamp, Point);

	// Convert to Unreal's FVector
	OutLabVector = FVector(LabVec[0], LabVec[1], LabVec[2]);

	return Error;
}

int32 FSuperTagGolfSwing::TransformFromLabToBody(FVector& OutBodyVector, const FVector& LabVector, int32 Timestamp, int32 PointLocation) const
{
	if (!IsValid())
	{
		OutBodyVector = FVector::ZeroVector;
		return -1;
	}

	GSVectorElement_t BodyVec[3] = {0, 0, 0};
	GSVectorElement_t LabVec[3] = {LabVector.X, LabVector.Y, LabVector.Z};

	// Create club point location from landmark
	GSClubPointLocation_t Point = GSMakeClubPointLocation(static_cast<GSClubLandmark_t>(PointLocation));

	GSErr Error = GSTransformFromLabToBody(SwingHandle, BodyVec, LabVec, Timestamp, Point);

	// Convert to Unreal's FVector
	OutBodyVector = FVector(BodyVec[0], BodyVec[1], BodyVec[2]);

	return Error;
}

int32 FSuperTagGolfSwing::TransformFromLabToScene(FVector& OutSceneVector, const FVector& LabVector) const
{
	if (!IsValid())
	{
		OutSceneVector = FVector::ZeroVector;
		return -1;
	}

	GSVectorElement_t SceneVec[3] = {0, 0, 0};
	GSVectorElement_t LabVec[3] = {LabVector.X, LabVector.Y, LabVector.Z};

	GSErr Error = GSTransformFromLabToScene(SwingHandle, SceneVec, LabVec);

	// Convert to Unreal's FVector
	OutSceneVector = FVector(SceneVec[0], SceneVec[1], SceneVec[2]);

	return Error;
}
