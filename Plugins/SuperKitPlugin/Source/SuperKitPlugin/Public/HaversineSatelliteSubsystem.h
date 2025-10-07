// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

// Include haversine library headers
#include "haversine/haversine_satellite_manager.h"
#include "haversine/haversine_satellite.h"
#include "haversine/haversine_satellite_state.h"
#include "haversine/haversine_environment.h"
#include "haversine/utils/events.h"

#include "Logging/LogMacros.h"
#include "HaversineSatelliteSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHaversineSatellite, Log, All);

class USuperTagAuthenticationManager;
class FSuperTagPermissionsDelegate;
class FSuperTagUpdateDelegate;

/**
 * Subsystem that manages Haversine satellite scanning and discovery
 * Auto-starts when game instance is created
 * Integrates SuperTag authentication and permissions
 */
UCLASS()
class SUPERKITPLUGIN_API UHaversineSatelliteSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Get the authentication manager
	 * @return The authentication manager instance
	 */
	USuperTagAuthenticationManager* GetAuthenticationManager() const { return AuthenticationManager; }

private:
	// Collection transfer delegate (defined in .cpp)
	class CollectionTransferDelegate;

	// Authentication manager (UObject)
	UPROPERTY()
	USuperTagAuthenticationManager* AuthenticationManager;

	// Satellite manager
	std::unique_ptr<haversine::HaversineSatelliteManager> SatelliteManager;

	// SuperTag delegates (owned by this subsystem, moved into environment)
	FSuperTagPermissionsDelegate* PermissionsDelegate;
	FSuperTagUpdateDelegate* UpdateDelegate;
	CollectionTransferDelegate* TransferDelegate;

	// Event subscriptions (RAII cleanup)
	std::unique_ptr<haversine::EventSubscription<haversine::BluetoothState>> BluetoothSubscription;
	std::unique_ptr<haversine::EventSubscription<std::shared_ptr<haversine::HaversineSatellite>>> DiscoverySubscription;
	std::unique_ptr<haversine::EventSubscription<haversine::Status>> ScanCompletionSubscription;

	// Helper functions
	void StartScanning();
	void OnBluetoothStateChanged(const haversine::BluetoothState& State);
	void OnSatelliteDiscovered(const std::shared_ptr<haversine::HaversineSatellite>& Satellite);
	void OnScanCompleted(const haversine::Status& Status);

	static FString FormatSatelliteState(const haversine::SatelliteState& State);
	static FString BluetoothStateToString(haversine::BluetoothState State);
};
