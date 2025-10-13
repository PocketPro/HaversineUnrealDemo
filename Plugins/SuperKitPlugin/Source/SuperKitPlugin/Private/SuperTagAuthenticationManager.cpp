// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTagAuthenticationManager.h"
#include "SuperTagTokenCache.h"
#include "SuperTagConfiguration.h"
#include "SuperKitPlugin.h"
#include "Http.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/Base64.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// Suppress warnings from third-party GolfSwingKit/PPCommon headers
THIRD_PARTY_INCLUDES_START
#ifdef _MSC_VER
#pragma warning(disable: 4068)  // unknown pragma mark
#pragma warning(disable: 4200)  // zero-sized array in struct
#pragma warning(disable: 4244)  // conversion warnings

// Fix DEPRECATED macro for MSVC (framework headers use GCC __attribute__)
#undef DEPRECATED
#define DEPRECATED(msg) __declspec(deprecated(msg))

// Fix __DEPRECATED_V1__ for MSVC
#undef __DEPRECATED_V1__
#define __DEPRECATED_V1__ __declspec(deprecated("For legacy SkyPro V1 data only"))
#endif

// Include GolfSwingKit authentication headers
#include "GolfSwing_authentication.h"

THIRD_PARTY_INCLUDES_END

// Type alias for cleaner code
using GSAuthTokenCache = GSAuthTokenCache_t;

USuperTagAuthenticationManager::USuperTagAuthenticationManager()
	: TokenCache(nullptr)
	, AuthTokenCacheHandle(nullptr)
	, RefetchInterval(FTimespan::FromHours(1.0))
	, ErrorRefetchInterval(FTimespan::FromSeconds(30.0))
	, TokenFetchEndpointPath(TEXT("/api4/skypro/get_features_for_tag.php"))
{
	// Create GolfSwingKit auth token cache handle
	AuthTokenCacheHandle = GSCreateAuthTokenCache();

	// Load token cache from disk
	TokenCache = USuperTagTokenCache::LoadOrCreate();
}

void USuperTagAuthenticationManager::BeginDestroy()
{
	// Free the GolfSwingKit auth token cache
	if (AuthTokenCacheHandle)
	{
		try
		{
			GSFreeAuthTokenCache(static_cast<GSAuthTokenCache*>(AuthTokenCacheHandle));
			AuthTokenCacheHandle = nullptr;
		}
		catch (...)
		{
			UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: Error freeing auth token cache"));
		}
	}

	// Cancel all pending HTTP requests
	for (auto& Request : PendingRequests)
	{
		if (Request->GetStatus() == EHttpRequestStatus::Processing)
		{
			Request->CancelRequest();
		}
	}
	PendingRequests.Empty();

	Super::BeginDestroy();
}

FString USuperTagAuthenticationManager::StandardizeHardwareId(const FString& HardwareId) const
{
	// Remove colons from hardware ID
	return HardwareId.Replace(TEXT(":"), TEXT(""));
}

void USuperTagAuthenticationManager::UpdateAuthenticationIfNecessary(const FString& HardwareId)
{
	FString StandardizedId = StandardizeHardwareId(HardwareId);

	// Check backoff logic - don't fetch too frequently
	if (NextFetchDateForHardwareId.Contains(StandardizedId))
	{
		FDateTime NextFetchDate = NextFetchDateForHardwareId[StandardizedId];
		if (FDateTime::Now() < NextFetchDate)
		{
			// Check if token is expiring soon (within 60 seconds)
			FString ExistingToken = CachedAuthenticationToken(StandardizedId);
			if (!ExistingToken.IsEmpty())
			{
				FDateTime ExpiryDate = GetTokenExpiryDate(ExistingToken);
				bool TokenExpiringSoon = (ExpiryDate - FDateTime::Now()).GetTotalSeconds() < 60.0;
				if (!TokenExpiringSoon)
				{
					return; // Token is still valid and not expiring soon
				}
			}
			else
			{
				return; // Within backoff period and no token exists
			}
		}
	}

	// Schedule next fetch
	NextFetchDateForHardwareId.Add(StandardizedId, FDateTime::Now() + RefetchInterval);

	// Start async token fetch
	FetchToken(StandardizedId);
}

FString USuperTagAuthenticationManager::CachedAuthenticationToken(const FString& HardwareId) const
{
	FString StandardizedId = StandardizeHardwareId(HardwareId);

	if (!TokenCache || !TokenCache->Tokens.Contains(StandardizedId))
	{
		return FString();
	}

	FString Token = TokenCache->Tokens[StandardizedId];

	// Check if token is expired
	FDateTime ExpiryDate = GetTokenExpiryDate(Token);
	if (ExpiryDate > FDateTime::Now())
	{
		return Token;
	}

	return FString();
}

bool USuperTagAuthenticationManager::HasCachedAuthenticationToken(const FString& HardwareId) const
{
	return !CachedAuthenticationToken(HardwareId).IsEmpty();
}

void USuperTagAuthenticationManager::FetchToken(const FString& HardwareId)
{
	// Construct URL with query parameters
	FString Url = FSuperTagConfiguration::SkyGolfBaseURL + TokenFetchEndpointPath;
	Url += TEXT("?dev=") + FGenericPlatformHttp::UrlEncode(FSuperTagConfiguration::SkygolfDevKey);
	Url += TEXT("&hardwareId=") + FGenericPlatformHttp::UrlEncode(HardwareId);

	UE_LOG(LogHaversineSatellite, Log, TEXT("SuperTagAuthenticationManager: Fetching token for hardware id %s"), *HardwareId);

	// Create HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Url);
	HttpRequest->SetVerb(TEXT("GET"));

	// Bind completion callback
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &USuperTagAuthenticationManager::OnTokenFetchComplete, HardwareId);

	// Track the request
	PendingRequests.Add(HttpRequest);

	// Send request
	if (!HttpRequest->ProcessRequest())
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: Failed to initiate HTTP request for %s"), *HardwareId);
		NextFetchDateForHardwareId.Add(HardwareId, FDateTime::Now() + ErrorRefetchInterval);
		PendingRequests.Remove(HttpRequest);
	}
}

void USuperTagAuthenticationManager::OnTokenFetchComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess, FString HardwareId)
{
	// Remove from pending requests
	PendingRequests.Remove(Request.ToSharedRef());

	if (!bSuccess || !Response.IsValid())
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: HTTP request failed for %s"), *HardwareId);
		NextFetchDateForHardwareId.Add(HardwareId, FDateTime::Now() + ErrorRefetchInterval);
		return;
	}

	int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode != 200)
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: HTTP error %d fetching token for %s"), ResponseCode, *HardwareId);
		NextFetchDateForHardwareId.Add(HardwareId, FDateTime::Now() + ErrorRefetchInterval);
		return;
	}

	// Parse JSON response
	FString ResponseString = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: Failed to parse JSON response for %s"), *HardwareId);
		NextFetchDateForHardwareId.Add(HardwareId, FDateTime::Now() + ErrorRefetchInterval);
		return;
	}

	// Extract response fields
	FString Code = JsonObject->GetStringField(TEXT("code"));
	FString Status = JsonObject->GetStringField(TEXT("status"));

	if (Code == TEXT("1153"))
	{
		// No membership found - this is not an error, just means no token available
		UE_LOG(LogHaversineSatellite, Log, TEXT("SuperTagAuthenticationManager: No token available for hardware id %s"), *HardwareId);
		SetToken(FString(), HardwareId); // Clear any existing token
	}
	else if (Code == TEXT("0"))
	{
		// Success
		FString Token = JsonObject->GetStringField(TEXT("data"));
		UE_LOG(LogHaversineSatellite, Log, TEXT("SuperTagAuthenticationManager: Successfully fetched token for hardware id %s"), *HardwareId);
		SetToken(Token, HardwareId);
	}
	else
	{
		// Error
		FString Message = JsonObject->HasField(TEXT("message")) ? JsonObject->GetStringField(TEXT("message")) : TEXT("Unknown error");
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: Token API returned error for hardware id %s: Code %s, Status: %s, Message: %s"),
			*HardwareId, *Code, *Status, *Message);
		NextFetchDateForHardwareId.Add(HardwareId, FDateTime::Now() + ErrorRefetchInterval);
	}
}

FDateTime USuperTagAuthenticationManager::GetTokenExpiryDate(const FString& Token) const
{
	try
	{
		// JWT format: header.payload.signature
		TArray<FString> Segments;
		Token.ParseIntoArray(Segments, TEXT("."));

		if (Segments.Num() != 3)
		{
			UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: Invalid JWT format (wrong number of segments) for token: %s"), *Token);
			return FDateTime::MinValue();
		}

		// Base64URL decode the payload segment
		FString PayloadSegment = Segments[1];

		// Convert Base64URL to standard Base64
		FString Base64 = PayloadSegment.Replace(TEXT("-"), TEXT("+")).Replace(TEXT("_"), TEXT("/"));

		// Add padding if needed
		int32 PaddingNeeded = Base64.Len() % 4;
		if (PaddingNeeded > 0)
		{
			Base64.Append(FString::ChrN(4 - PaddingNeeded, '='));
		}

		// Decode Base64
		TArray<uint8> PayloadBytes;
		if (!FBase64::Decode(Base64, PayloadBytes))
		{
			UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: Failed to decode Base64 payload for token: %s"), *Token);
			return FDateTime::MinValue();
		}

		// Convert bytes to string
		FString PayloadJson = FString(UTF8_TO_TCHAR(PayloadBytes.GetData()));

		// Parse JSON payload
		TSharedPtr<FJsonObject> PayloadObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(PayloadJson);

		if (!FJsonSerializer::Deserialize(Reader, PayloadObject) || !PayloadObject.IsValid())
		{
			UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: Failed to parse JWT payload JSON for token: %s"), *Token);
			return FDateTime::MinValue();
		}

		// Extract 'exp' claim (Unix timestamp)
		if (!PayloadObject->HasField(TEXT("exp")))
		{
			UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: JWT payload missing 'exp' claim for token: %s"), *Token);
			return FDateTime::MinValue();
		}

		double ExpUnixTimestamp = PayloadObject->GetNumberField(TEXT("exp"));
		return FDateTime::FromUnixTimestamp(static_cast<int64>(ExpUnixTimestamp));
	}
	catch (...)
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: Error parsing JWT token"));
		return FDateTime::MinValue();
	}
}

void USuperTagAuthenticationManager::SetToken(const FString& Token, const FString& HardwareId)
{
	if (!TokenCache)
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagAuthenticationManager: TokenCache is null, cannot save token"));
		return;
	}

	if (Token.IsEmpty())
	{
		// Remove token
		TokenCache->Tokens.Remove(HardwareId);
	}
	else
	{
		// Set token
		TokenCache->Tokens.Add(HardwareId, Token);
	}

	// Persist to disk
	TokenCache->Save();
}
