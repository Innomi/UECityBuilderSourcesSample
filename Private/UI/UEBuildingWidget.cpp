// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UEBuildingWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Building/UEBuildingDataAsset.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "GameModes/UEGameState.h"
#include "Path/UEPathDataAsset.h"
#include "Player/UEPlayerController.h"

void UUEBuildingWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UUEBuildingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	TObjectPtr<UWorld> World = GetWorld();
	if (!IsValid(BuildingPanel) || !IsValid(World))
	{
		return;
	}
	TObjectPtr<AUEGameState> GameState = World->GetGameState<AUEGameState>();
	if (!IsValid(GameState))
	{
		return;
	}
	TObjectPtr<AUEPlayerController> UEPlayerController = GetOwningPlayer<AUEPlayerController>();
	auto AddButton = [this] <typename DataAssetType> (DataAssetType const & DataAsset)
		{
			TObjectPtr<AUEPlayerController> UEPlayerController = GetOwningPlayer<AUEPlayerController>();
			TObjectPtr<UButton> Button = WidgetTree->ConstructWidget<UButton>();
			TObjectPtr<UImage> Image = WidgetTree->ConstructWidget<UImage>();
			Image->SetBrush(DataAsset.Icon);
			Button->AddChild(Image);
			SButton & SlateButton = static_cast<SButton &>(Button->TakeWidget().Get());
			auto const ActorClass = DataAsset.ActorClass;
			SlateButton.SetOnClicked(FOnClicked::CreateLambda([UEPlayerController, ActorClass]()
				{
					if (IsValid(UEPlayerController))
					{
						UEPlayerController->BeginPlacement(ActorClass);
					}
					return FReply::Handled();
				}));
			BuildingPanel->AddChild(Button);
		};
	if (TObjectPtr<UUEPathDataAsset const> RoadDataAsset = GameState->GetRoadDataAsset(); IsValid(RoadDataAsset))
	{
		AddButton(*RoadDataAsset);
	}
	for (TObjectPtr<UUEBuildingDataAsset const> BuildingDataAsset : GameState->GetBuildingDataAssets())
	{
		if (!IsValid(BuildingDataAsset))
		{
			continue;
		}
		AddButton(*BuildingDataAsset);
	}
}