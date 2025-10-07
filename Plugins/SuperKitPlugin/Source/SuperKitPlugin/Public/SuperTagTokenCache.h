// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SuperTagTokenCache.generated.h"

/**
 * SaveGame object for persisting authentication tokens across sessions
 * Stores a mapping of hardware IDs to JWT tokens
 */
UCLASS()
class SUPERKITPLUGIN_API USuperTagTokenCache : public USaveGame
{
	GENERATED_BODY()

public:
	/** Map of hardware ID to authentication token */
	UPROPERTY()
	TMap<FString, FString> Tokens;

	/** Save slot name for this cache */
	static const FString SaveSlotName;

	/** User index for save game system */
	static const int32 UserIndex;

	/** Load the token cache from disk, or create a new one if it doesn't exist */
	static USuperTagTokenCache* LoadOrCreate();

	/** Save the token cache to disk */
	void Save();
};
