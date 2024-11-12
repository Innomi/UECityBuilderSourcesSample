// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UEBuildingWidget.generated.h"

class UPanelWidget;

/**
 * Widget to provide selection of placeables for placement.
 */
UCLASS(Abstract)
class UNDEADEMPIRE_API UUEBuildingWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPanelWidget> BuildingPanel;
};
