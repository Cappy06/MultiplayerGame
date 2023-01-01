// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 *
 */
UCLASS()
class MAING_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)); // associates C++ variable with tthe text block in the widget blueprint
	class UTextBlock *DisplayText;
	void SetDisplayText(FString TextToDisplay);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn *InPawn);

	UFUNCTION(BlueprintCallable)
	void ShowName(APawn *InPawn);

protected:
	virtual void OnLevelRemovedFromWorld(ULevel *InLevel, UWorld *InWorld) override;
};
