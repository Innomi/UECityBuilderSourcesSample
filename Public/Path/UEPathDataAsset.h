// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Common/UEPlaceableDataAsset.h"
#include "CoreMinimal.h"
#include "Path/UEPathActor.h"
#include "UEPathDataAsset.generated.h"

/**
 * UUEPathDataAsset
 */
UCLASS()
class UNDEADEMPIRE_API UUEPathDataAsset : public UUEPlaceableDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "UE")
	TSubclassOf<AUEPathActor> ActorClass;
};