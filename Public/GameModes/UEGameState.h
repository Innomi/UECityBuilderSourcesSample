// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Path/UEPathSystem.h"
#include "UEGameState.generated.h"

class UUEBuildingDataAsset;
class UUEPathDataAsset;

/**
 * AUEGameState
 */
UCLASS()
class UNDEADEMPIRE_API AUEGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	TArray<TObjectPtr<UUEBuildingDataAsset>> const & GetBuildingDataAssets() const;
	TObjectPtr<UUEPathDataAsset const> GetRoadDataAsset() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UE")
	TArray<TObjectPtr<UUEBuildingDataAsset>> BuildingDataAssets;

	UPROPERTY(EditDefaultsOnly, Category = "UE")
	TObjectPtr<UUEPathDataAsset> RoadDataAsset;
};