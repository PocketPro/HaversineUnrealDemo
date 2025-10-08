// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Forward declarations (GolfSwingKit is C, avoid including in header)
struct GSSwing_t_;
typedef struct GSSwing_t_ GSSwing_t;
struct GSAuthTokenCache_s;
typedef struct GSAuthTokenCache_s GSAuthTokenCache_t;

/**
 * RAII wrapper around GolfSwingKit's GSSwing_t
 * Provides safe construction/destruction and exposes raw handle for direct GolfSwingKit API access
 *
 * Usage:
 *   FSuperTagGolfSwing swing(collectionData, authToken, tokenCache);
 *
 *   // Option 1: Use convenience methods
 *   float speed = swing.GetClubheadSpeed();
 *
 *   // Option 2: Direct GolfSwingKit C API access
 *   GSGetParameterForKey(swing.GetHandle(), kSomeParameter);
 */
class SUPERKITPLUGIN_API FSuperTagGolfSwing
{
public:
	/**
	 * Create and parse a golf swing from raw collection data
	 * @param CollectionData Raw swing data from satellite
	 * @param AuthenticationToken Token for authenticating the swing data
	 * @param TokenCache Optional cache for authentication tokens (can be nullptr)
	 * @throws Will log error and create invalid swing if parsing fails
	 */
	FSuperTagGolfSwing(const std::vector<uint8_t>& CollectionData,
	                   const FString& AuthenticationToken,
	                   GSAuthTokenCache_t* TokenCache);

	/**
	 * Parse hardware ID from raw collection data without creating a full swing
	 * @param CollectionData Raw swing data from satellite
	 * @return Hardware ID string, or empty string on error
	 */
	static FString ParseHardwareId(const std::vector<uint8_t>& CollectionData);

	// Destructor - automatically frees the GSSwing_t handle
	~FSuperTagGolfSwing();

	// No copy (GSSwing_t is unique)
	FSuperTagGolfSwing(const FSuperTagGolfSwing&) = delete;
	FSuperTagGolfSwing& operator=(const FSuperTagGolfSwing&) = delete;

	// Move semantics
	FSuperTagGolfSwing(FSuperTagGolfSwing&& Other) noexcept;
	FSuperTagGolfSwing& operator=(FSuperTagGolfSwing&& Other) noexcept;

	/**
	 * Get the raw GSSwing_t handle for direct GolfSwingKit C API access
	 * Allows calling any GSGetXXX() function directly
	 * @return Raw handle, may be nullptr if construction failed
	 */
	GSSwing_t* GetHandle() const { return SwingHandle; }

	/**
	 * Check if this swing is valid (construction succeeded)
	 */
	bool IsValid() const { return SwingHandle != nullptr; }

	//
	// Convenience methods (wrapper around common GolfSwingKit functions)
	// Users can also call GolfSwingKit directly via GetHandle()
	//

	/**
	 * Get clubhead speed in MPH
	 * @return Speed in MPH, or 0.0 if invalid
	 */
	float GetClubheadSpeed() const;

	/**
	 * Get the club type for this swing
	 * @return Club identifier string
	 */
	FString GetClub() const;

	/**
	 * Get the sensor identifier (hardware ID)
	 * @return Sensor identifier string
	 */
	FString GetSensorIdentifier() const;

	/**
	 * Check if this is a right-handed swing
	 * @return true if right-handed, false if left-handed
	 */
	bool IsRightHanded() const;

	/**
	 * Check if this is an airswing (practice swing with no ball contact)
	 * @return true if airswing, false if ball was hit
	 */
	bool IsAirswing() const;

	//
	// Swing Timing Methods - Get timestamps for key swing events
	// All times are in milliseconds
	//

	/**
	 * Get timestamp when swing starts
	 * @return Time in microseconds
	 */
	int32 GetTimeSwingStart() const;

	/**
	 * Get timestamp at mid-backswing
	 * @return Time in microseconds
	 */
	int32 GetTimeMidBackswing() const;

	/**
	 * Get timestamp at top of backswing
	 * @return Time in microseconds
	 */
	int32 GetTimeTopOfBackswing() const;

	/**
	 * Get timestamp at mid-downswing
	 * @return Time in microseconds
	 */
	int32 GetTimeMidDownswing() const;

	/**
	 * Get timestamp at impact
	 * @return Time in microseconds
	 */
	int32 GetTimeImpact() const;

	/**
	 * Get timestamp when swing ends
	 * @return Time in microseconds
	 */
	int32 GetTimeSwingEnd() const;

	/**
	 * Get the Unix timestamp when this swing was recorded
	 * @return Unix timestamp in seconds, or 0 if unavailable
	 */
	uint32 GetSwingTimestamp() const;

	//
	// Swing Parameters - Access any swing metric by parameter key
	//

	/**
	 * Get a specific swing parameter by key
	 * See GolfSwing_types.h for available GSParameterKey_t values
	 * Common keys: attack angle, path direction, face angle, tempo, etc.
	 *
	 * @param Key The parameter key (e.g., GSParameterKeyAttackAngleImpact)
	 * @param OutValue The parameter value (in appropriate units based on parameter type)
	 * @param OutStatus Error status (0 = success)
	 * @return true if parameter was retrieved successfully
	 */
	bool GetParameterForKey(int32 Key, float& OutValue, int32& OutStatus) const;


	//
	// Position and Orientation Methods - Club position and coordinate transforms
	//

	/**
	 * Get the origin of the scene coordinate frame in lab frame
	 * Scene frame origin is at the club's address position (leading edge center)
	 *
	 * @param OutOrigin Origin position [x, y, z] in meters
	 * @return 0 on success, error code otherwise
	 */
	int32 GetSceneOriginLab(FVector& OutOrigin) const;

	/**
	 * Get club position at a specific time in lab frame
	 *
	 * @param OutPosition Position vector [x, y, z] in meters
	 * @param PointLocation Which point on the club (butt, clip, hand, tip, center face)
	 * @param Timestamp Time in milliseconds
	 * @return 0 on success, error code otherwise
	 */
	int32 GetPositionLab(FVector& OutPosition, int32 PointLocation, int32 Timestamp) const;

	/**
	 * Get club position at a specific time in scene frame
	 * Scene frame is translated so origin is at club address position
	 *
	 * @param OutPosition Position vector [x, y, z] in meters
	 * @param PointLocation Which point on the club (butt, clip, hand, tip, center face)
	 * @param Timestamp Time in milliseconds
	 * @return 0 on success, error code otherwise
	 */
	int32 GetPositionScene(FVector& OutPosition, int32 PointLocation, int32 Timestamp) const;

	/**
	 * Get the club head path distance traveled at a specific time
	 * This is the arc length along the club head path from start of swing
	 *
	 * @param OutDistance Path distance in meters
	 * @param Timestamp Time in milliseconds
	 * @return 0 on success, error code otherwise
	 */
	int32 GetClubheadPathDistance(float& OutDistance, int32 Timestamp) const;

	/**
	 * Get the timestamp when the club head has traveled a specific path distance
	 * Inverse of GetClubheadPathDistance
	 *
	 * @param OutTimestamp Output timestamp in milliseconds
	 * @param PathDistance Desired path distance in meters
	 * @return 0 on success, error code otherwise
	 */
	int32 GetTimeForClubHeadPathDistance(int32& OutTimestamp, float PathDistance) const;

	//
	// Coordinate Transformation Methods
	//

	/**
	 * Get the transformation matrix from body frame to lab frame at a specific time
	 * Body frame is fixed to the club, lab frame is the world coordinate system
	 *
	 * @param OutMatrix 3x3 rotation matrix (9 elements in row-major order)
	 * @param Timestamp Time in milliseconds
	 * @param PointLocation Which point on the club
	 * @return 0 on success, error code otherwise
	 */
	int32 GetBodyToLabMatrix(TArray<double>& OutMatrix, int32 Timestamp, int32 PointLocation) const;

	/**
	 * Transform a vector from body frame to lab frame at a specific time
	 *
	 * @param OutLabVector Output vector in lab frame
	 * @param BodyVector Input vector in body frame
	 * @param Timestamp Time in milliseconds
	 * @param PointLocation Which point on the club
	 * @return 0 on success, error code otherwise
	 */
	int32 TransformFromBodyToLab(FVector& OutLabVector, const FVector& BodyVector, int32 Timestamp, int32 PointLocation) const;

	/**
	 * Transform a vector from lab frame to body frame at a specific time
	 *
	 * @param OutBodyVector Output vector in body frame
	 * @param LabVector Input vector in lab frame
	 * @param Timestamp Time in milliseconds
	 * @param PointLocation Which point on the club
	 * @return 0 on success, error code otherwise
	 */
	int32 TransformFromLabToBody(FVector& OutBodyVector, const FVector& LabVector, int32 Timestamp, int32 PointLocation) const;

	/**
	 * Transform a vector from lab frame to scene frame
	 * Scene frame origin is at club address position
	 *
	 * @param OutSceneVector Output vector in scene frame
	 * @param LabVector Input vector in lab frame
	 * @return 0 on success, error code otherwise
	 */
	int32 TransformFromLabToScene(FVector& OutSceneVector, const FVector& LabVector) const;

private:
	GSSwing_t* SwingHandle;
};
