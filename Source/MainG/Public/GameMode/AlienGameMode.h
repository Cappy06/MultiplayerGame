// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "AlienGameMode.generated.h"

/**
 *
 */
UCLASS()
class MAING_API AAlienGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual void PlayerEliminated(class AAlienCharacter *ElimmedCharacter, class AAlienPlayerController *VictimController, AAlienPlayerController *AttackerController);
	virtual void RequestRespawn(ACharacter *ElimmedCharacter, AController *ElimmedController);
};
