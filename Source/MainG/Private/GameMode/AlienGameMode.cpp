// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/AlienGameMode.h"
#include "Character/AlienCharacter.h"
#include "PlayerController/AlienPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/AlienPlayerState.h"

void AAlienGameMode::PlayerEliminated(class AAlienCharacter *ElimmedCharacter, class AAlienPlayerController *VictimController, AAlienPlayerController *AttackerController)
{
    AAlienPlayerState *AttackerPlayerState = AttackerController ? Cast<AAlienPlayerState>(AttackerController->PlayerState) : nullptr;
    AAlienPlayerState *VictimPlayerState = VictimController ? Cast<AAlienPlayerState>(VictimController->PlayerState) : nullptr;

    if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
    {
        AttackerPlayerState->AddToScore(1.0f);
    }
    if (VictimPlayerState)
    {
        VictimPlayerState->AddToDefeats(1);
    }
    if (ElimmedCharacter)
    {
        ElimmedCharacter->Elim();
    }
}

// respawning after killing
void AAlienGameMode::RequestRespawn(ACharacter *ElimmedCharacter, AController *ElimmedController)
{
    if (ElimmedCharacter)
    {
        ElimmedCharacter->Reset();
        ElimmedCharacter->Destroy();
    }
    if (ElimmedController)
    {
        TArray<AActor *> PlayerStarts;
        UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
        int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
        RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
    }
}