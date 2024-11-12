// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UEResourceDataAsset.generated.h"

/**
 * UUEResourceDataAsset
 */
UCLASS()
class UNDEADEMPIRE_API UUEResourceDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "UE|Resource")
	FName ResourceName;

	UPROPERTY(EditAnywhere, Category = "UE|Resource")
	FSlateBrush ResourceIcon;
};
