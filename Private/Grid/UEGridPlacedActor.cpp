// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEGridPlacedActor.h"
#include "Grid/UEGridPlacementComponent.h"

AUEGridPlacedActor::AUEGridPlacedActor()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("RootComponent"));

	GridPlacement = CreateDefaultSubobject<UUEGridPlacementComponent>("GridPlacement");
	GridPlacement->SetupAttachment(RootComponent);
}