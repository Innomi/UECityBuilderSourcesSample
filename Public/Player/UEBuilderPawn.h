// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/UETopDownCameraPawn.h"
#include "UEBuilderPawn.generated.h"

/**
 * AUEBuilderPawn
 */
UCLASS()
class UNDEADEMPIRE_API AUEBuilderPawn : public AUETopDownCameraPawn
{
	GENERATED_BODY()
	
public:
	AUEBuilderPawn();

	virtual void NotifyControllerChanged() override;

	void ShowUI();
	void HideUI();

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> BuildingWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> BuildingWidget;
};
