// Fill out your copyright notice in the Description page of Project Settings.

#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
    if (DisplayText)
    {
        DisplayText->SetText(FText::FromString(TextToDisplay));
    }
}

// Network Role - for identifying the roles of the online players, which is server, which is client, who are you, who are the players
void UOverheadWidget::ShowPlayerNetRole(APawn *InPawn)
{
    ENetRole LocalRole = InPawn->GetLocalRole();
    FString Role;
    switch (LocalRole)
    {
    case ENetRole::ROLE_Authority:
        Role = FString("Authority");
        break;
    case ENetRole::ROLE_AutonomousProxy:
        Role = FString("AutonomousProxy");
        break;
    case ENetRole::ROLE_SimulatedProxy:
        Role = FString("SimulatedProxy");
        break;
    case ENetRole::ROLE_None:
        Role = FString("None");
        break;
    }
    FString LocalRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
    SetDisplayText(LocalRoleString);
}
// by me
void UOverheadWidget::ShowName(APawn *InPawn)
{

    FString PName = InPawn->GetName();
    SetDisplayText(PName);
}
void UOverheadWidget::OnLevelRemovedFromWorld(ULevel *InLevel, UWorld *InWorld)
{
    RemoveFromParent();
    Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}
