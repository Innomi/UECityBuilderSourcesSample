// Fill out your copyright notice in the Description page of Project Settings.

#include "Building/UEBuildingActor.h"
#include "Building/UEBuildingComponent.h"
#include "Grid/UEGridPlacementComponent.h"

AUEBuildingActor::AUEBuildingActor()
{
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(GridPlacement, UUEGridPlacementComponent::GridCenterSocketName);
	UEBuilding = CreateDefaultSubobject<UUEBuildingComponent>(TEXT("UEBuilding"));
	RootComponent->SetMobility(EComponentMobility::Static);
	GridPlacement->SetMobility(EComponentMobility::Static);
	StaticMesh->SetMobility(EComponentMobility::Static);
	StaticMesh->SetCollisionProfileName("NoCollision");
}