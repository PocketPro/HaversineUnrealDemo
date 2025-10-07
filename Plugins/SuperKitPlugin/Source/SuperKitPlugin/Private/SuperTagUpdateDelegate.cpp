// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTagUpdateDelegate.h"
#include "haversine/haversine_satellite.h"

std::unique_ptr<haversine::HaversineFirmware> FSuperTagUpdateDelegate::firmware_update(const haversine::HaversineSatellite& Satellite)
{
	// Return nullptr if no update is set
	// Otherwise, we would need to clone/copy the firmware update
	// For now, firmware updates are not implemented
	return nullptr;
}

std::unique_ptr<haversine::HaversineSensorConfig> FSuperTagUpdateDelegate::sensor_config_update(const haversine::HaversineSatellite& Satellite)
{
	// Return nullptr if no update is set
	// Otherwise, we would need to clone/copy the sensor config update
	// For now, sensor config updates are not implemented
	return nullptr;
}

void FSuperTagUpdateDelegate::SetFirmwareUpdate(std::unique_ptr<haversine::HaversineFirmware> Update)
{
	FirmwareUpdatePtr = std::move(Update);
}

void FSuperTagUpdateDelegate::SetSensorConfigUpdate(std::unique_ptr<haversine::HaversineSensorConfig> Update)
{
	SensorConfigUpdatePtr = std::move(Update);
}
