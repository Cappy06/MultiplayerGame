// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerState/AlienPlayerState.h"
#include "Character/AlienCharacter.h"
#include "PlayerController/AlienPlayerController.h"
#include "Net/UnrealNetwork.h"

void AAlienPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAlienPlayerState, Defeats);
}
void AAlienPlayerState::AddToScore(float ScoreAmount)
{

    // Score += ScoreAmount;
    SetScore(GetScore() + ScoreAmount);
    Character = Character == nullptr ? Cast<AAlienCharacter>(GetPawn()) : Character;
    if (Character)
    {
        Controller = Controller == nullptr ? Cast<AAlienPlayerController>(Character->Controller) : Controller;
        if (Controller)
        {
            Controller->SetHUDScore(GetScore());
        }
    }
}

void AAlienPlayerState::OnRep_Score()
{
    Super::OnRep_Score();
    Character = Character == nullptr ? Cast<AAlienCharacter>(GetPawn()) : Character;
    if (Character)
    {
        Controller = Controller == nullptr ? Cast<AAlienPlayerController>(Character->Controller) : Controller;
        if (Controller)
        {
            Controller->SetHUDScore(GetScore());
        }
    }
}
void AAlienPlayerState::AddToDefeats(int32 DefeatsAmount)
{
    // Score += ScoreAmount;
    Defeats += DefeatsAmount;
    Character = Character == nullptr ? Cast<AAlienCharacter>(GetPawn()) : Character;
    if (Character)
    {
        Controller = Controller == nullptr ? Cast<AAlienPlayerController>(Character->Controller) : Controller;
        if (Controller)
        {
            Controller->SetHUDDefeats(Defeats);
        }
    }
}

void AAlienPlayerState::OnRep_Defeats()
{
    Character = Character == nullptr ? Cast<AAlienCharacter>(GetPawn()) : Character;
    if (Character)
    {
        Controller = Controller == nullptr ? Cast<AAlienPlayerController>(Character->Controller) : Controller;
        if (Controller)
        {
            Controller->SetHUDDefeats(Defeats);
        }
    }
}
