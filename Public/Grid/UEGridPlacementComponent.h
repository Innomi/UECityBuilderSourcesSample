// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UEGridPlacementComponent.generated.h"

enum class EUEGridLayer : uint8;
class UUEGridSystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNDEADEMPIRE_API UUEGridPlacementComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUEGridPlacementComponent();

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	
	virtual bool HasAnySockets() const override;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const override;
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription> & OutSockets) const override;

	FIntPoint GetGridSize() const;
	
	/**
	 * Set grid size to register for this component. Causes a component re-register if the component is already registered.
	 */
	void SetGridSize(FIntPoint const InGridSize);
	
	FIntPoint GetLocationOnGrid() const;
	FIntRect GetGridRect(FIntPoint const LocationOnGrid) const;

	EUEGridLayer GetLayerToRegisterOn() const;
	
	/**
	 * Set grid layer to register on for this component. Causes a component re-register if the component is already registered.
	 */
	void SetLayerToRegisterOn(EUEGridLayer const GridLayer);

	bool IsRegisteredOnGrid() const;
	bool ShouldBeRegisteredOnGrid() const;
	void SetShouldBeRegisteredOnGrid(bool const bInShouldBeRegisteredOnGrid);

	/**
	 * Checks if rectangle can be placed on grid layer specified to register for this component.
	 */
	bool CanBePlacedOnGrid(FIntRect const & Rect) const;
	/**
	 * Checks if rectangle can be placed on specified grid layer.
	 */
	bool CanBePlacedOnGrid(EUEGridLayer const GridLayer, FIntRect const & Rect) const;

	FVector GridSnapLocation(FVector const & InLocation) const;

	/**
	 * The name of the socket at the center of grid.
	 */
	static FName const GridCenterSocketName;

protected:
	void SetOccupationOnGrid(EUEGridLayer const GridLayer, FIntPoint const Coords, bool const bIsOccupied) const;
	void SetOccupationOnGrid(EUEGridLayer const GridLayer, FIntRect const & Rect, bool const bIsOccupied) const;

	UPROPERTY(EditDefaultsOnly, Category = "Grid Placement")
	FIntPoint GridSize;

	UPROPERTY(EditDefaultsOnly, Category = "Grid Placement")
	EUEGridLayer GridLayerToRegisterOn;

	UPROPERTY(VisibleAnywhere, Category = "Grid Placement")
	uint8 bIsRegisteredOnGrid : 1;

	/**
	 * Whether to register this component in system.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Grid Placement")
	uint8 bShouldBeRegisteredOnGrid : 1;
};
