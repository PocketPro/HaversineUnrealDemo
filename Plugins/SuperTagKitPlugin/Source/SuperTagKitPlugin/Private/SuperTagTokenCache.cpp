// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTagTokenCache.h"
#include "Kismet/GameplayStatics.h"
#include "SuperTagKitPlugin.h"

const FString USuperTagTokenCache::SaveSlotName = TEXT("SuperTagTokenCache");
const int32 USuperTagTokenCache::UserIndex = 0;

USuperTagTokenCache* USuperTagTokenCache::LoadOrCreate()
{
	USuperTagTokenCache* LoadedCache = nullptr;

	// Try to load existing save game
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
	{
		LoadedCache = Cast<USuperTagTokenCache>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));

		if (LoadedCache)
		{
			UE_LOG(LogHaversineSatellite, Log, TEXT("SuperTagTokenCache: Loaded %d cached tokens from disk"), LoadedCache->Tokens.Num());
			return LoadedCache;
		}
		else
		{
			UE_LOG(LogHaversineSatellite, Warning, TEXT("SuperTagTokenCache: Failed to load existing save game, creating new cache"));
		}
	}

	// Create new cache if load failed or doesn't exist
	LoadedCache = Cast<USuperTagTokenCache>(UGameplayStatics::CreateSaveGameObject(USuperTagTokenCache::StaticClass()));
	UE_LOG(LogHaversineSatellite, Log, TEXT("SuperTagTokenCache: Created new token cache"));

	return LoadedCache;
}

void USuperTagTokenCache::Save()
{
	if (UGameplayStatics::SaveGameToSlot(this, SaveSlotName, UserIndex))
	{
		UE_LOG(LogHaversineSatellite, Log, TEXT("SuperTagTokenCache: Saved %d tokens to disk"), Tokens.Num());
	}
	else
	{
		UE_LOG(LogHaversineSatellite, Error, TEXT("SuperTagTokenCache: Failed to save tokens to disk"));
	}
}
