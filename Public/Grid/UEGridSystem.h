// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grid/UEGridLayer.h"
#include "Subsystems/WorldSubsystem.h"
#include "UEGridSystem.generated.h"

class UUEGridComponent;

UENUM(BlueprintType)
enum class EUEGridLayer : uint8
{
	NatureObstacle,
	Construction,
	Road,
	LAYERS_NUM UMETA(Hidden)
};

ENUM_RANGE_BY_COUNT(EUEGridLayer, EUEGridLayer::LAYERS_NUM)

UCLASS()
class UNDEADEMPIRE_API UUEGridSystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UUEGridSystem();

	void RegisterGridComponent(TObjectPtr<UUEGridComponent> const GridComponent);
	void UnregisterGridComponent(TObjectPtr<UUEGridComponent> const GridComponent);

	TObjectPtr<UUEGridComponent> GetGridComponent(FVector2D const Coords) const;
	TObjectPtr<UUEGridComponent> GetGridComponent(FIntPoint const Coords) const;
	TArray<TObjectPtr<UUEGridComponent>> GetGridComponents(FBox2D const & Rect) const;
	TArray<TObjectPtr<UUEGridComponent>> GetGridComponents(FIntRect const Rect) const;

	float GetCellSize() const;
	FIntPoint GetCellCoords(FVector2D const Coords) const;
	FIntRect GetIntRect(FBox2D const & Rect) const;
	bool IsInGrid(FIntPoint const CellCoords) const;

	/** Checks if cell containing point Coords is occupied. */
	bool IsCellOccupied(EUEGridLayer const GridLayer, FVector2D const Coords) const;
	bool IsCellOccupied(EUEGridLayer const GridLayer, FIntPoint const CellCoords) const;
	
	/** Sets cell containing point Coords state. */
	bool SetCellState(EUEGridLayer const GridLayer, FVector2D const Coords, bool const bIsOccupied);
	bool SetCellState(EUEGridLayer const GridLayer, FIntPoint const CellCoords, bool const bIsOccupied);

	/** Checks if specified rectangle is containing occupied cell. */
	bool HasOccupiedCell(EUEGridLayer const GridLayer, FBox2D const & Rect) const;
	bool HasOccupiedCell(EUEGridLayer const GridLayer, FIntRect const & Rect) const;

	/** Sets cells state in specified rectangle. */
	void SetCellsState(EUEGridLayer const GridLayer, FBox2D const & Rect, bool const bIsOccupied);
	void SetCellsState(EUEGridLayer const GridLayer, FIntRect const & Rect, bool const bIsOccupied);

private:
	// TODO: in general case here should be spatial tree index.
	TArray<TObjectPtr<UUEGridComponent>> GridComponents;
	TArray<FIntRect> GridRects;
	float CellSize;
};
