// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "haversine/haversine_environment.h"

// Forward declarations from haversine namespace
namespace haversine {
	class HaversineFirmware;
	class HaversineSensorConfig;
}

/**
 * Delegate for providing firmware and sensor config updates to satellites
 */
class SUPERTAGKITPLUGIN_API FSuperTagUpdateDelegate : public haversine::HaversineUpdateDelegate
{
public:
	FSuperTagUpdateDelegate() = default;
	virtual ~FSuperTagUpdateDelegate() = default;

	// HaversineUpdateDelegate interface
	virtual std::unique_ptr<haversine::HaversineFirmware> firmware_update(const haversine::HaversineSatellite& Satellite) override;
	virtual std::unique_ptr<haversine::HaversineSensorConfig> sensor_config_update(const haversine::HaversineSatellite& Satellite) override;

	/**
	 * Set the firmware update to distribute to satellites
	 * This class takes ownership of the firmware object
	 *
	 * @param Update The firmware update (nullptr to disable firmware updates)
	 */
	void SetFirmwareUpdate(std::unique_ptr<haversine::HaversineFirmware> Update);

	/**
	 * Set the sensor config update to distribute to satellites
	 * This class takes ownership of the config object
	 *
	 * @param Update The sensor config update (nullptr to disable config updates)
	 */
	void SetSensorConfigUpdate(std::unique_ptr<haversine::HaversineSensorConfig> Update);

private:
	/** Optional firmware update to provide to satellites */
	std::unique_ptr<haversine::HaversineFirmware> FirmwareUpdatePtr;

	/** Optional sensor config update to provide to satellites */
	std::unique_ptr<haversine::HaversineSensorConfig> SensorConfigUpdatePtr;
};
