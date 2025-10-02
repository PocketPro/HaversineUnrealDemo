// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealTest1GameMode.h"
#include "UnrealTest1Character.h"
#include "UObject/ConstructorHelpers.h"

AUnrealTest1GameMode::AUnrealTest1GameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
