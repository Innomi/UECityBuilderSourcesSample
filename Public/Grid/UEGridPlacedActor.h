// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Common/UEActor.h"
#include "UEGridPlacedActor.generated.h"

class UUEGridPlacementComponent;

/**
 * AUEGridPlacementActor
 */
UCLASS()
class UNDEADEMPIRE_API AUEGridPlacedActor : public AUEActor
{
	GENERATED_BODY()
	
public:
	AUEGridPlacedActor();

protected:
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UUEGridPlacementComponent> GridPlacement;
};
