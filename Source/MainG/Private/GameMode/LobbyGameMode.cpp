// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController *NewPlayer)
{
    // when we override the function we call superclass
    Super::PostLogin(NewPlayer);
    // GameState have array of players that have joined tha game
    int32 NumberOfPlayers = GameState->PlayerArray.Num();
    if (NumberOfPlayers == 2)
    {
        UWorld *World = GetWorld();
        if (World)
        {
            bUseSeamlessTravel = true;
            World->ServerTravel(FString("/Game/Maps/AnaimalaiHills?listen"));
        }
    }
}
