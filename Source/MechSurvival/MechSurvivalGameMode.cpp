// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MechSurvivalGameMode.h"
#include "MechSurvivalHUD.h"
#include "MechSurvivalCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMechSurvivalGameMode::AMechSurvivalGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AMechSurvivalHUD::StaticClass();
}
