// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Building/UEBuildingActor.h"
#include "Common/UEPlaceableDataAsset.h"
#include "CoreMinimal.h"
#include "UEBuildingDataAsset.generated.h"

/**
 * UUEBuildingDataAsset
 */
UCLASS()
class UNDEADEMPIRE_API UUEBuildingDataAsset : public UUEPlaceableDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "UE")
	TSubclassOf<AUEBuildingActor> ActorClass;
};