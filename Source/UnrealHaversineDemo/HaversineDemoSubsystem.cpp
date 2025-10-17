// Copyright Epic Games, Inc. All Rights Reserved.

//
// HaversineDemoSubsystem.cpp
// UnrealHaversineDemo
//
// Unreal Engine Game Instance Subsystem for detecting SuperTag satellites
// and transferring golf swing collections via Bluetooth LE
//

#include "HaversineDemoSubsystem.h"
#include "SuperTagAuthenticationManager.h"
#include "SuperTagPermissionsDelegate.h"
#include "SuperTagUpdateDelegate.h"
#include "SuperTagExtensions.h"
#include "SuperTagGolfSwing.h"
#include "haversine/haversine_satellite_manager.h"
#include "haversine/haversine_environment.h"
#include "haversine/haversine_satellite.h"
#include "haversine/haversine_satellite_state.h"
#include "haversine/satellite_id.h"
#include "haversine/utils/events.h"

// Forward declaration for GolfSwingKit types
struct GSAuthTokenCache_s;
typedef struct GSAuthTokenCache_s GSAuthTokenCache_t;

//
// Nested Delegate Classes
//

/// # Collection Transfer Delegate
/// A `HaversineCollectionTransferDelegate` is an object that controls the transfer of collections (i.e. swings).
///
/// If a satellite is handled (see `HaversinePermissionsDelegate`), the SDK will call these methods to allow customization of the transfer
/// and to receive collection data (or an error code if the transfer fails).
///
/// Some notes about collections & transferring:
/// - Every collection has an `index`. It is used as an identifier in the methods below.  It starts 0 and increments over the lifetime of the satellite, rolling over at 2^16.
/// - There may be multiple collections stored on the satellite by the time we connect to it. `first_collection_to_transfer` allows some or all of these to be transferred.
/// - Satellites do not support "random access" of collections.  This means you always receive swings with montonically increasing `indexes`.
/// - If you decide not to transfer a collection in `first_collection_to_transfer`, you will not be given the chance to access it again.
///   - This is because transfers are cheaper than connection setup. If you think you might need a swing later, it's better to transfer it now, even if you don't process it immediately.
/// - If a transfer fails while processing a range of indexes, those indexes that were not successfully transferred will be automatically re-attempted as soon as possible.
class UHaversineDemoSubsystem::CollectionTransferDelegate : public haversine::HaversineCollectionTransferDelegate
{
public:
	explicit CollectionTransferDelegate(USuperTagAuthenticationManager* InAuthManager)
		: AuthManager(InAuthManager)
	{
	}

	virtual uint16_t first_collection_to_transfer(
		const haversine::CollectionIndexes& Range,
		const haversine::SatelliteId& SatelliteId) override
	{
		// For this demo, transfer the last swing (most recent)
		// To transfer all, return Range.start_index
		// To transfer none, return Range.end_index
		FString SatID = UTF8_TO_TCHAR(SatelliteId.str().c_str());
		UE_LOG(LogHaversineSatellite, Log, TEXT("  → Starting collection transfer from index %d to %d for satellite %s"),
			Range.start_index, Range.end_index, *SatID);
		return Range.end_index - 1; // Transfer last swing only
	}

	virtual void will_transfer_collections(
		const haversine::CollectionIndexes& Range,
		const haversine::SatelliteId& SatelliteId) override
	{
        // Optional: could uodate UI if we want to indicate a swing transfer starting.
		FString SatID = UTF8_TO_TCHAR(SatelliteId.str().c_str());
		UE_LOG(LogHaversineSatellite, Log, TEXT("  → Will transfer %d collections from satellite %s"),
			Range.end_index - Range.start_index, *SatID);
	}

	virtual void collection_transfer_did_finish(
		const std::vector<uint8_t>& CollectionData,
		uint16_t CollectionIndex,
		const haversine::SatelliteId& SatelliteId) override
	{
        // A collection transfer completed successfully.
        // - We now use the physics engine to process `collection_data` into a Golfswing.
        // - This requires authentication with SkyGolf API as shown below.

		FString SatID = UTF8_TO_TCHAR(SatelliteId.str().c_str());
		UE_LOG(LogHaversineSatellite, Log, TEXT("  ✓ Collection %d transferred successfully (%d bytes) from satellite %s"),
			CollectionIndex, CollectionData.size(), *SatID);

		// Parse hardware ID from swing data
		FString HardwareId = FSuperTagGolfSwing::ParseHardwareId(CollectionData);
		if (HardwareId.IsEmpty())
		{
			UE_LOG(LogHaversineSatellite, Error, TEXT("  ✗ Failed to parse hardware ID from swing data"));
			return;
		}

		// Get authentication token for this hardware
		if (!AuthManager)
		{
			UE_LOG(LogHaversineSatellite, Error, TEXT("  ✗ AuthManager is null, cannot process swing"));
			return;
		}

		FString AuthToken = AuthManager->CachedAuthenticationToken(HardwareId);
		if (AuthToken.IsEmpty())
		{
			UE_LOG(LogHaversineSatellite, Error, TEXT("  ✗ No authentication token for satellite %s, swing discarded"), *HardwareId);
			return;
		}

		// Create and parse swing object
        // *** This is where we get an actual golf swing with metrics! ***
		GSAuthTokenCache_t* TokenCache = static_cast<GSAuthTokenCache_t*>(AuthManager->GetAuthTokenCacheHandle());
		FSuperTagGolfSwing Swing(CollectionData, AuthToken, TokenCache);
		if (!Swing.IsValid())
		{
			UE_LOG(LogHaversineSatellite, Error, TEXT("  ✗ Swing failed reconstruction from satellite %s"), *HardwareId);
			return;
		}

		// For now, we just log some example properties of the swing.  See `SuperTagGolfSwing.h` for more information
		FString ClubName = Swing.GetClub();
		float Speed = Swing.GetClubheadSpeed();
		FString Handedness = Swing.IsRightHanded() ? TEXT("Right") : TEXT("Left");
		UE_LOG(LogHaversineSatellite, Log, TEXT("  ✓ Swing processed: Club=%s, Speed=%.1f MPH, %s"),
			*ClubName, Speed, *Handedness);

		// Display on-screen message
		if (GEngine)
		{
			FString Message = FString::Printf(TEXT("Swing: %s @ %.1f MPH (%s)"), *ClubName, Speed, *Handedness);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, Message);
		}
	}

	virtual void collection_transfer_did_fail(
		const haversine::Status& Error,
		uint16_t CollectionIndex,
		const haversine::SatelliteId& SatelliteId) override
	{
        // A transfer failed. It will automatically be re-attempted, but if you modified UI or changed state in `will_transfer_collections`,
        // you may want to clean it up here.
		FString SatID = UTF8_TO_TCHAR(SatelliteId.str().c_str());
		FString ErrorMsg = UTF8_TO_TCHAR(Error.to_string().c_str());
		UE_LOG(LogHaversineSatellite, Error, TEXT("  ✗ Collection %d transfer failed: %s for satellite %s"),
			CollectionIndex, *ErrorMsg, *SatID);
	}

private:
	USuperTagAuthenticationManager* AuthManager;
};

//
// UHaversineDemoSubsystem Implementation
//
// This method sets up the SDK and configures it to scan for satellites.
void UHaversineDemoSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogHaversineSatellite, Warning, TEXT("*** HAVERSINE SATELLITE SUBSYSTEM STARTING ***"));
	UE_LOG(LogHaversineSatellite, Log, TEXT("Initializing Haversine Satellite Subsystem"));

	// On-screen debug message
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Haversine Satellite Subsystem Initialized!"));
	}

	// Create an authentication manager. This is used to authenticate swings for processing.
	AuthenticationManager = NewObject<USuperTagAuthenticationManager>(this);

	// A `PermissionDelegate` is an object that tells the HaversineSatelliteLibrary SDK which
    // satellites (supertags) to interact with.
	PermissionsDelegate = new FSuperTagPermissionsDelegate(AuthenticationManager);

    // An `UpdateDelegate` can be configured to update the firmware on the supertags if necessary.
    // This is unlikely to be used, and you can probably just ignore it.
	UpdateDelegate = new FSuperTagUpdateDelegate();

    // We've seen the collection transfer delegate above; it is the object that handles collection (swing) transfer.
	TransferDelegate = new CollectionTransferDelegate(AuthenticationManager);

    // Now create a "HaversineEnvironment" with these delegates.
    // A "HaversineEnviroment" is the type used to customize SDK behaviour for a fleet of satellites. It holds
    // the permissions and transfer delegate we discussed earlier, but it also has a number of other options
    // for customization. One important one, not implemented here, is the ability to provide a persistant cache
    // which will prevent unnecessary connections to satellites across app launches.
	haversine::HaversineEnvironment Environment;
	Environment.set_permissions_delegate(std::unique_ptr<haversine::HaversinePermissionsDelegate>(PermissionsDelegate));
	Environment.set_update_delegate(std::unique_ptr<haversine::HaversineUpdateDelegate>(UpdateDelegate));
	Environment.set_transfer_delegate(std::unique_ptr<haversine::HaversineCollectionTransferDelegate>(TransferDelegate));

    // Create a "HaversineSatelliteManager". This is the top-level object for working with HaversineSatellites.
    // - It takes the environment and also the hardware version of the SuperTag satellites. This should be 10.0
	UE_LOG(LogHaversineSatellite, Log, TEXT("Creating satellite manager (HW version 10.0)"));
	SatelliteManager = std::make_unique<haversine::HaversineSatelliteManager>(
		std::move(Environment), 10, 0
	);

    // The `manager` will publish various events...

	// Subscribe to Bluetooth state changes. We can only start scanning when Bluetooth is powered on.
	BluetoothSubscription = std::make_unique<haversine::EventSubscription<haversine::BluetoothState>>(
		const_cast<haversine::EventChannel<haversine::BluetoothState>&>(SatelliteManager->bluetooth_state_events())
			.subscribe([this](const haversine::BluetoothState& State) {
				OnBluetoothStateChanged(State);
			})
	);

	// Subscribe to satellite discoveries.
    // - This happens when a new nearby satellite (supertag) is discovered while scanning.
    // - For subsequent state updates for this supertag, see `state_update_events` on the discovered satellite.
	DiscoverySubscription = std::make_unique<haversine::EventSubscription<std::shared_ptr<haversine::HaversineSatellite>>>(
		const_cast<haversine::EventChannel<std::shared_ptr<haversine::HaversineSatellite>>&>(SatelliteManager->discovery_events())
			.subscribe([this](const std::shared_ptr<haversine::HaversineSatellite>& Satellite) {
				OnSatelliteDiscovered(Satellite);
			})
	);

	// Subscribe to scan completion
    // - This is called when you stop scanning, or if it completes with an error (e.g. Bluetooth turned off)
	ScanCompletionSubscription = std::make_unique<haversine::EventSubscription<haversine::Status>>(
		const_cast<haversine::EventChannel<haversine::Status>&>(SatelliteManager->scanning_completion_events())
			.subscribe([this](const haversine::Status& Status) {
				OnScanCompleted(Status);
			})
	);

	// Check current Bluetooth state and start scanning if possible.
	haversine::BluetoothState CurrentState = SatelliteManager->bluetooth_state();
	UE_LOG(LogHaversineSatellite, Log, TEXT("Current Bluetooth state: %s"), *BluetoothStateToString(CurrentState));

	if (CurrentState == haversine::BluetoothState::PoweredOn)
	{
		StartScanning();
	}
	else
	{
		UE_LOG(LogHaversineSatellite, Warning, TEXT("Bluetooth not ready yet, waiting for PoweredOn state..."));
	}
}


void UHaversineDemoSubsystem::StartScanning()
{
	if (!SatelliteManager)
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("Cannot start scanning: SatelliteManager is null"));
		return;
	}

	if (SatelliteManager->is_scanning())
	{
		UE_LOG(LogHaversineSatellite, Log, TEXT("Already scanning, skipping start request"));
		return;
	}

	UE_LOG(LogHaversineSatellite, Log, TEXT("Starting satellite scan..."));

	haversine::Status ScanResult = SatelliteManager->scan_for_satellites();
	if (!ScanResult.ok())
	{
		FString ErrorMsg = UTF8_TO_TCHAR(ScanResult.to_string().c_str());
		UE_LOG(LogHaversineSatellite, Error, TEXT("Failed to start scanning: %s"), *ErrorMsg);
	}
	else
	{
		UE_LOG(LogHaversineSatellite, Log, TEXT("Scanning started successfully"));
	}
}

void UHaversineDemoSubsystem::OnBluetoothStateChanged(const haversine::BluetoothState& State)
{
	UE_LOG(LogHaversineSatellite, Log, TEXT("Bluetooth State: %s"), *BluetoothStateToString(State));

	// Auto-start scanning when Bluetooth becomes ready
	if (State == haversine::BluetoothState::PoweredOn && SatelliteManager && !SatelliteManager->is_scanning())
	{
		UE_LOG(LogHaversineSatellite, Log, TEXT("Bluetooth powered on, auto-starting scan"));
		StartScanning();
	}
}

void UHaversineDemoSubsystem::OnSatelliteDiscovered(const std::shared_ptr<haversine::HaversineSatellite>& Satellite)
{
	if (!Satellite)
	{
		UE_LOG(LogHaversineSatellite, Warning, TEXT("Received null satellite in discovery event"));
		return;
	}

	FString SatelliteID = UTF8_TO_TCHAR(Satellite->id().str().c_str());
	FString SatelliteName = Satellite->name()
		? UTF8_TO_TCHAR(Satellite->name()->c_str())
		: TEXT("(unnamed)");
	FString StateInfo = FormatSatelliteState(Satellite->state());

	// Try to parse metadata with authentication
	FString ClubInfo = TEXT("none");
	FString UserInfo = TEXT("none");
	if (AuthenticationManager)
	{
		haversine::Result<FSuperTagMetadata> MetadataResult = FSuperTagExtensions::ParseMetadata(Satellite->state(), AuthenticationManager);

		if (MetadataResult.ok())
		{
			const FSuperTagMetadata& Metadata = MetadataResult.value();

			if (Metadata.Club.IsSet())
			{
				const GSClub& Club = Metadata.Club.GetValue();
				FString ClubLongName = UTF8_TO_TCHAR(Club.longName);
				ClubInfo = ClubLongName.IsEmpty() ? TEXT("(unnamed club)") : ClubLongName;
			}

			if (Metadata.UserId.IsSet())
			{
				UserInfo = FString::Printf(TEXT("User %u"), Metadata.UserId.GetValue());
			}
		}
		else
		{
			FString ErrorMsg = UTF8_TO_TCHAR(MetadataResult.status().to_string().c_str());
			ClubInfo = FString::Printf(TEXT("parse error: %s"), *ErrorMsg);
		}
	}

	UE_LOG(LogHaversineSatellite, Log, TEXT("🛰️  Discovered: %s (%s) - %s | Club: %s | User: %s"),
		*SatelliteID, *SatelliteName, *StateInfo, *ClubInfo, *UserInfo);
}

void UHaversineDemoSubsystem::OnScanCompleted(const haversine::Status& Status)
{
	if (Status.ok())
	{
		UE_LOG(LogHaversineSatellite, Log, TEXT("Scanning completed successfully"));
	}
	else
	{
		FString ErrorMsg = UTF8_TO_TCHAR(Status.to_string().c_str());
		UE_LOG(LogHaversineSatellite, Error, TEXT("Scanning completed with error: %s"), *ErrorMsg);
	}
}

void UHaversineDemoSubsystem::Deinitialize()
{
    UE_LOG(LogHaversineSatellite, Log, TEXT("Shutting down Haversine Satellite Subsystem"));

    if (SatelliteManager && SatelliteManager->is_scanning())
    {
        UE_LOG(LogHaversineSatellite, Log, TEXT("Stopping active scan..."));
        SatelliteManager->stop_scanning();
    }

    // Print final summary
    if (SatelliteManager)
    {
        auto Discovered = SatelliteManager->get_discovered_satellites();
        UE_LOG(LogHaversineSatellite, Log, TEXT("Final summary: %d satellites discovered"), Discovered.size());

        for (const auto& [ID, Satellite] : Discovered)
        {
            FString SatID = UTF8_TO_TCHAR(ID.str().c_str());
            FString Name = Satellite->name()
                ? UTF8_TO_TCHAR(Satellite->name()->c_str())
                : TEXT("(unnamed)");
            FString StateInfo = FormatSatelliteState(Satellite->state());
            UE_LOG(LogHaversineSatellite, Log, TEXT("  • %s (%s) - %s"), *SatID, *Name, *StateInfo);
        }
    }

    // RAII will cleanup subscriptions and manager
    BluetoothSubscription.reset();
    DiscoverySubscription.reset();
    ScanCompletionSubscription.reset();
    SatelliteManager.reset();

    Super::Deinitialize();
}

FString UHaversineDemoSubsystem::FormatSatelliteState(const haversine::SatelliteState& State)
{
	// Movement/collecting status
	FString MovementState;
	if (State.transient().inCollectionState)
	{
		MovementState = TEXT("collecting");
	}
	else if (State.transient().isMoving)
	{
		MovementState = TEXT("moving");
	}
	else
	{
		MovementState = TEXT("still");
	}

	// Firmware version
	FString FirmwareVersion = FString::Printf(TEXT("FW:%d.%d"),
		State.persistent().platform_versions().firmwareVersionMajor,
		State.persistent().platform_versions().firmwareVersionMinor);

	// Status indicators
	TArray<FString> StatusIcons;
	if (State.transient().isDark)
	{
		StatusIcons.Add(TEXT("☾"));
	}
	else
	{
		StatusIcons.Add(TEXT("☀"));
	}

	if (State.transient().needsServicing)
	{
		StatusIcons.Add(TEXT("⚠"));
	}

	if (State.transient().hasDebugInfo)
	{
		StatusIcons.Add(TEXT("☠"));
	}

	// Collections
	FString Collections = FString::Printf(TEXT("%d collections"),
		State.truncated_collection_count());

	// Combine all parts
	FString StatusIconsStr = FString::Join(StatusIcons, TEXT(" "));
	return FString::Printf(TEXT("[%s] | %s | %s | %s"),
		*MovementState, *FirmwareVersion, *StatusIconsStr, *Collections);
}

FString UHaversineDemoSubsystem::BluetoothStateToString(haversine::BluetoothState State)
{
	switch (State)
	{
		case haversine::BluetoothState::PoweredOn:
			return TEXT("PoweredOn ✓");
		case haversine::BluetoothState::PoweredOff:
			return TEXT("PoweredOff ✗");
		case haversine::BluetoothState::Unsupported:
			return TEXT("Unsupported ✗");
		case haversine::BluetoothState::Unauthorized:
			return TEXT("Unauthorized ✗");
		case haversine::BluetoothState::Unknown:
			return TEXT("Unknown");
		case haversine::BluetoothState::Resetting:
			return TEXT("Resetting");
		default:
			return TEXT("Invalid");
	}
}
