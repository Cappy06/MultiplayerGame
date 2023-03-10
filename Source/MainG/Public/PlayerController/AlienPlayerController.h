// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AlienPlayerController.generated.h"

/**
 *
 */
UCLASS()
class MAING_API AAlienPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	virtual void OnPossess(APawn *InPawn) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class AAlienHUD *AlienHUD;
};
