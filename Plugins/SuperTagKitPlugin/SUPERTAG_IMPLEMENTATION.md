# SuperTag Implementation Summary

This document describes the Unity SuperTagKit components that have been ported to the Unreal Engine SuperKitPlugin.

## Overview

The SuperTag system provides JWT-based authentication, metadata parsing, user filtering, and swing data processing for Haversine satellites. All core components from the Unity implementation have been ported to Unreal C++.

## Implemented Components

### 1. FSuperTagConfiguration
**Files:** `SuperTagConfiguration.h/cpp`

Static configuration class storing API endpoints and keys:
- `SkyGolfBaseURL`: QA environment API endpoint
- `SkygolfDevKey`: Developer authentication key

### 2. FSuperTagMetadata
**Files:** `SuperTagMetadata.h/cpp`

Wrapper for GolfSwingKit's `GSSensorMetadata`:
- `Club` (TOptional<GSClub>): Associated club information
- `UserId` (TOptional<uint32>): Associated user ID
- `Timestamp` (FDateTime): When metadata was created

### 3. USuperTagTokenCache
**Files:** `SuperTagTokenCache.h/cpp`

UE SaveGame object for persistent token storage:
- Stores `TMap<FString, FString>` mapping hardware IDs to JWT tokens
- Automatically saves/loads from disk
- Uses save slot "SuperTagTokenCache"

### 4. USuperTagAuthenticationManager
**Files:** `SuperTagAuthenticationManager.h/cpp`

UObject managing JWT token lifecycle:
- **HTTP Requests**: Fetches tokens from SkyGolf API using Unreal's HTTP module
- **JWT Parsing**: Extracts expiry date from token payload
- **Token Caching**: Persistent storage via USuperTagTokenCache
- **Backoff Logic**: Prevents excessive API requests (1 hour normal, 30 sec on error)
- **GolfSwingKit Integration**: Manages native auth token cache handle

**Key Methods:**
- `UpdateAuthenticationIfNecessary(HardwareId)`: Async token fetch with backoff
- `CachedAuthenticationToken(HardwareId)`: Get valid cached token
- `HasCachedAuthenticationToken(HardwareId)`: Check token availability
- `GetAuthTokenCacheHandle()`: Get native GolfSwingKit cache pointer

### 5. FSuperTagExtensions
**Files:** `SuperTagExtensions.h/cpp`

Static utility functions for metadata parsing:
- `ParseMetadata(SatelliteState, AuthToken, Cache)`: Parse with explicit token
- `ParseMetadata(SatelliteState, AuthManager)`: Parse using auth manager
- Wraps GolfSwingKit's `GSSensorMetadata_deserialize()`

### 6. FSuperTagPermissionsDelegate
**Files:** `SuperTagPermissionsDelegate.h/cpp`

Implements `haversine::HaversinePermissionsDelegate` with user filtering:

**Permission Modes:**
- `AnyUser`: Accept all satellites
- `MultiUser`: Filter by specific user ID list

**Methods:**
- `should_handle_advertisement()`: Pre-filter using fingerprint matching
- `should_handle_satellite()`: Full metadata check with authentication
- `should_transfer_collections()`: User-based transfer control
- `SetSingleUserMode(UserId)`: Configure for single user
- `SetMultiUserMode(UserIds)`: Configure for multiple users
- `SetAnyUserMode()`: Disable filtering

### 7. FSuperTagUpdateDelegate
**Files:** `SuperTagUpdateDelegate.h/cpp`

Implements `haversine::IHaversineUpdateDelegate` for firmware/config updates:
- `get_firmware_update()`: Provide firmware updates to satellites
- `get_sensor_config_update()`: Provide config updates to satellites
- `SetFirmwareUpdate()`: Configure firmware update
- `SetSensorConfigUpdate()`: Configure config update

### 8. UHaversineSatelliteSubsystem (Updated)
**Files:** `HaversineSatelliteSubsystem.h/cpp`

Integrated all SuperTag components:
- Creates `USuperTagAuthenticationManager` instance
- Uses `FSuperTagPermissionsDelegate` for user filtering
- Uses `FSuperTagUpdateDelegate` for updates
- Custom `CollectionTransferDelegate` with auth token injection
- Parses and logs club/user metadata on satellite discovery
- Ready for swing data processing (when GolfSwingKit is linked)

## GolfSwingKit Integration

The implementation includes stub declarations for GolfSwingKit types and functions:

**Stub Structures:**
- `GSClub`, `GSUser`, `GSSensorMetadata`

**Stub Functions:**
- `GSCreateAuthTokenCache()` / `GSFreeAuthTokenCache()`
- `GSSensorMetadata_deserialize()`
- `GSSensorMetadata_fingerprintMatchesUserId()`
- `GSSensorMetadata_fingerprintMatchesNoUser()`

**TODO:** Replace stubs with actual GolfSwingKit headers when the C++ library is linked.

## Build Configuration

Added required Unreal modules to `SuperKitPlugin.Build.cs`:
- `HTTP`: For authentication HTTP requests
- `Json`: For JSON parsing
- `JsonUtilities`: For JSON serialization

## Authentication Flow

1. **Discovery**: Satellite discovered via Bluetooth
2. **Pre-filter**: `should_handle_advertisement()` uses fingerprint matching
3. **Token Fetch**: `UpdateAuthenticationIfNecessary()` fetches JWT if needed
4. **Metadata Parse**: `ParseMetadata()` decrypts satellite metadata
5. **Permission Check**: `should_handle_satellite()` verifies user ID
6. **Transfer**: `should_transfer_collections()` approves swing transfer
7. **Processing**: Swing data processed with auth token (when GolfSwingKit linked)

## API Endpoint

Token fetch endpoint:
```
GET https://qa2-8264ee52f589f4c0191aa94f87aa1aeb-sg360.skygolf.com/api4/skypro/get_features_for_tag.php?dev={key}&hardwareId={id}
```

**Response Codes:**
- `"0"`: Success (token in `data` field)
- `"1153"`: No membership found (not an error)
- Other: Error (check `status` and `message` fields)

## Token Caching

Tokens are cached in two places:
1. **Disk**: Via `USuperTagTokenCache` SaveGame (persistent across sessions)
2. **Memory**: Via GolfSwingKit's native auth token cache (performance optimization)

## Next Steps

1. **Link GolfSwingKit**: Replace stub implementations with actual C++ library
2. **Test Authentication**: Verify token fetching and caching works
3. **Test User Filtering**: Verify permission delegate filters correctly
4. **Implement Swing Processing**: Parse transferred swing data using GolfSwingKit
5. **Add Blueprints**: Expose functionality to Blueprint if needed

## File Structure

```
Plugins/SuperKitPlugin/Source/SuperKitPlugin/
├── Public/
│   ├── SuperTagConfiguration.h
│   ├── SuperTagMetadata.h
│   ├── SuperTagTokenCache.h
│   ├── SuperTagAuthenticationManager.h
│   ├── SuperTagExtensions.h
│   ├── SuperTagPermissionsDelegate.h
│   ├── SuperTagUpdateDelegate.h
│   └── HaversineSatelliteSubsystem.h (updated)
└── Private/
    ├── SuperTagConfiguration.cpp
    ├── SuperTagMetadata.cpp
    ├── SuperTagTokenCache.cpp
    ├── SuperTagAuthenticationManager.cpp
    ├── SuperTagExtensions.cpp
    ├── SuperTagPermissionsDelegate.cpp
    ├── SuperTagUpdateDelegate.cpp
    └── HaversineSatelliteSubsystem.cpp (updated)
```

## Differences from Unity

1. **Language**: C# → C++ (Unreal's coding standards)
2. **Async**: Unity's `async/await` → Unreal's HTTP delegates
3. **Persistence**: Unity's `PlayerPrefs` → Unreal's SaveGame system
4. **JSON**: Unity's `JsonUtility` → Unreal's `FJsonSerializer`
5. **Types**: C# types → Unreal types (`FString`, `TOptional`, `FDateTime`, etc.)
6. **Memory**: C# GC → Unreal's UObject system + manual lifetime management

## Current Status

✅ All 7 Unity components ported
✅ Authentication manager with HTTP/JWT support
✅ Token caching with SaveGame
✅ Permission filtering with user ID support
✅ Metadata parsing integration
✅ Build configuration updated
⏳ Waiting for GolfSwingKit C++ library linkage
⏳ Swing data processing (requires GolfSwingKit)
