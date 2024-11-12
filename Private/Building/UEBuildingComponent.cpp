// Fill out your copyright notice in the Description page of Project Settings.

#include "Building/UEBuildingComponent.h"
#include "Building/UEBuildingSystem.h"
#include "Grid/UEGridPlacementComponent.h"

void UUEBuildingComponent::OnRegister()
{
	Super::OnRegister();

	TObjectPtr<AActor> const Owner = GetOwner();
	// Movable actor can't be registered as building.
	if (Owner && !Owner->IsRootComponentMovable())
	{
		TObjectPtr<UUEGridPlacementComponent> const GridPlacementComponent = Owner->GetComponentByClass<UUEGridPlacementComponent>();
		TObjectPtr<UUEBuildingSystem> const BuildingSystem = GetBuildingSystem();
		if (GridPlacementComponent && BuildingSystem)
		{
			BuildingSystem->AddBuildingAsync(Owner, GridPlacementComponent->GetGridRect(GridPlacementComponent->GetLocationOnGrid()));
		}
	}
}

void UUEBuildingComponent::OnUnregister()
{
	TObjectPtr<AActor> const Owner = GetOwner();
	// Movable actor can't be registered as building.
	if (Owner && !Owner->IsRootComponentMovable())
	{
		TObjectPtr<UUEGridPlacementComponent> const GridPlacementComponent = Owner->GetComponentByClass<UUEGridPlacementComponent>();
		TObjectPtr<UUEBuildingSystem> const BuildingSystem = GetBuildingSystem();
		if (GridPlacementComponent && BuildingSystem)
		{
			BuildingSystem->RemoveOverlappedBuildingsAsync(GridPlacementComponent->GetGridRect(GridPlacementComponent->GetLocationOnGrid()), Owner);
		}
	}

	Super::OnUnregister();
}

TObjectPtr<UUEBuildingSystem> UUEBuildingComponent::GetBuildingSystem() const
{
	if (TObjectPtr<UWorld> const World = GetWorld())
	{
		return World->GetSubsystem<UUEBuildingSystem>();
	}
	return nullptr;
}