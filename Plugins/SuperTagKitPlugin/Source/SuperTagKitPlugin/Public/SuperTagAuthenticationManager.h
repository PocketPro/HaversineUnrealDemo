// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SuperTagAuthenticationManager.generated.h"

class USuperTagTokenCache;
class IHttpRequest;
class IHttpResponse;

// Type aliases for HTTP shared pointers
using FHttpRequestPtr = TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>;
using FHttpResponsePtr = TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>;

/**
 * Manager for handling authentication with Haversine satellites
 * Fetches and caches JWT tokens from the SkyGolf API
 *
 * Thread Safety: All public methods should be called from the game thread
 */
UCLASS()
class SUPERTAGKITPLUGIN_API USuperTagAuthenticationManager : public UObject
{
	GENERATED_BODY()

public:
	USuperTagAuthenticationManager();

	/**
	 * A handle to memory used by the GolfSwingKit to cache parsed authentication tokens
	 * This is a native pointer that GolfSwingKit uses for performance optimization
	 */
	void* GetAuthTokenCacheHandle() const { return AuthTokenCacheHandle; }

	/**
	 * Fetches and caches the authentication token for a given hardware ID if it hasn't been fetched recently
	 * Uses backoff logic to prevent excessive API requests
	 *
	 * @param HardwareId The hardware ID (serial number) of the satellite
	 */
	void UpdateAuthenticationIfNecessary(const FString& HardwareId);

	/**
	 * Retrieves the cached authentication token for the specified hardware ID
	 * Returns empty string if no valid token is cached
	 *
	 * @param HardwareId The hardware ID to look up
	 * @return The JWT token if valid and cached, empty string otherwise
	 */
	FString CachedAuthenticationToken(const FString& HardwareId) const;

	/**
	 * Checks if a valid authentication token is currently cached for the specified hardware ID
	 *
	 * @param HardwareId The hardware ID to check
	 * @return True if a valid token exists, false otherwise
	 */
	bool HasCachedAuthenticationToken(const FString& HardwareId) const;

protected:
	virtual void BeginDestroy() override;

private:
	/** Token cache stored on disk */
	UPROPERTY()
	USuperTagTokenCache* TokenCache;

	/** Native handle for GolfSwingKit's authentication token cache */
	void* AuthTokenCacheHandle;

	/** Tracks the next allowed fetch time for each hardware ID (backoff logic) */
	TMap<FString, FDateTime> NextFetchDateForHardwareId;

	/** Standard interval between token fetches */
	FTimespan RefetchInterval;

	/** Shorter interval for retrying after errors */
	FTimespan ErrorRefetchInterval;

	/** API endpoint path for token fetching */
	FString TokenFetchEndpointPath;

	/** Pending HTTP requests (to keep them alive) */
	TArray<TSharedRef<IHttpRequest, ESPMode::ThreadSafe>> PendingRequests;

	/**
	 * Standardizes hardware ID format (removes colons)
	 *
	 * @param HardwareId The hardware ID to standardize
	 * @return Standardized hardware ID
	 */
	FString StandardizeHardwareId(const FString& HardwareId) const;

	/**
	 * Initiates an asynchronous token fetch from the API
	 *
	 * @param HardwareId The hardware ID to fetch a token for
	 */
	void FetchToken(const FString& HardwareId);

	/**
	 * Callback when HTTP request completes
	 *
	 * @param Request The HTTP request object
	 * @param Response The HTTP response object
	 * @param bSuccess Whether the request succeeded
	 * @param HardwareId The hardware ID this request was for
	 */
	void OnTokenFetchComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess, FString HardwareId);

	/**
	 * Extracts the expiration date from a JWT token
	 *
	 * @param Token The JWT token string
	 * @return The expiration date, or FDateTime::MinValue() if parsing fails
	 */
	FDateTime GetTokenExpiryDate(const FString& Token) const;

	/**
	 * Saves a token to the cache and persists to disk
	 * If token is empty, removes the entry
	 *
	 * @param Token The token to save (empty to remove)
	 * @param HardwareId The hardware ID associated with this token
	 */
	void SetToken(const FString& Token, const FString& HardwareId);
};
