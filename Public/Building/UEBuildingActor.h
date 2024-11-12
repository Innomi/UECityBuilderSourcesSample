// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grid/UEGridPlacedActor.h"
#include "UEBuildingActor.generated.h"

class UUEBuildingComponent;

/**
 * AUEBuildingActor
 */
UCLASS()
class UNDEADEMPIRE_API AUEBuildingActor : public AUEGridPlacedActor
{
	GENERATED_BODY()
	
public:
	AUEBuildingActor();

protected:
	UPROPERTY(VisibleDefaultsOnly, Category = "UE")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(VisibleDefaultsOnly, Category = "UE")
	TObjectPtr<UUEBuildingComponent> UEBuilding;
};
