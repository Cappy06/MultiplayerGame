// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerController/AlienPlayerController.h"
#include "HUD/AlienHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Character/AlienCharacter.h"

void AAlienPlayerController::BeginPlay()
{
    Super::BeginPlay();

    AlienHUD = Cast<AAlienHUD>(GetHUD());
}

void AAlienPlayerController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);
    AAlienCharacter *AlienCharacter = Cast<AAlienCharacter>(InPawn);
    if (AlienCharacter)
    {
        SetHUDHealth(AlienCharacter->GetHealth(), AlienCharacter->GetMaxHealth());
    }
}
void AAlienPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
    AlienHUD = AlienHUD == nullptr ? Cast<AAlienHUD>(GetHUD()) : AlienHUD;

    bool bHUDValid = AlienHUD &&
                     AlienHUD->CharacterOverlay &&
                     AlienHUD->CharacterOverlay->HealthBar &&
                     AlienHUD->CharacterOverlay->HealthText;
    if (bHUDValid)
    {
        const float HealthPercent = Health / MaxHealth;
        AlienHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
        FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
        AlienHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
    }
}
void AAlienPlayerController::SetHUDScore(float Score)
{
    AlienHUD = AlienHUD == nullptr ? Cast<AAlienHUD>(GetHUD()) : AlienHUD;
    bool bHUDValid = AlienHUD &&
                     AlienHUD->CharacterOverlay &&
                     AlienHUD->CharacterOverlay->ScoreAmount;

    if (bHUDValid)
    {
        FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
        AlienHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
    }
}

void AAlienPlayerController::SetHUDDefeats(int32 Defeats)
{
    AlienHUD = AlienHUD == nullptr ? Cast<AAlienHUD>(GetHUD()) : AlienHUD;
    bool bHUDValid = AlienHUD &&
                     AlienHUD->CharacterOverlay &&
                     AlienHUD->CharacterOverlay->DefeatsAmount;

    if (bHUDValid)
    {
        FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
        AlienHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
    }
}