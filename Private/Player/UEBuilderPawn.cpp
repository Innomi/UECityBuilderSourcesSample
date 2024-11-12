// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/UEBuilderPawn.h"
#include "UI/UEBuildingWidget.h"

AUEBuilderPawn::AUEBuilderPawn()
{
	BuildingWidget = nullptr;
	BuildingWidgetClass = nullptr;
}

void AUEBuilderPawn::NotifyControllerChanged()
{
	HideUI();
	ShowUI();

	Super::NotifyControllerChanged();
}

void AUEBuilderPawn::ShowUI()
{
	if (TObjectPtr<APlayerController> PlayerController = Cast<APlayerController>(GetController()); IsValid(PlayerController))
	{
		BuildingWidget = CreateWidget<UUserWidget>(PlayerController, BuildingWidgetClass);
		if (BuildingWidget)
		{
			BuildingWidget->AddToViewport();
		}
	}
}

void AUEBuilderPawn::HideUI()
{
	if (IsValid(BuildingWidget))
	{
		BuildingWidget->RemoveFromParent();
		BuildingWidget = nullptr;
	}
}