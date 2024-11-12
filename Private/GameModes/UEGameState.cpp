// Fill out your copyright notice in the Description page of Project Settings.

#include "GameModes/UEGameState.h"

TArray<TObjectPtr<UUEBuildingDataAsset>> const & AUEGameState::GetBuildingDataAssets() const
{
	return BuildingDataAssets;
}

TObjectPtr<UUEPathDataAsset const> AUEGameState::GetRoadDataAsset() const
{
	return RoadDataAsset;
}