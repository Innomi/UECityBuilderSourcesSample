// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UEPlaceableDataAsset.generated.h"

/**
 * UUEPlaceableDataAsset
 */
UCLASS(Abstract)
class UNDEADEMPIRE_API UUEPlaceableDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "UE")
	FSlateBrush Icon;
};