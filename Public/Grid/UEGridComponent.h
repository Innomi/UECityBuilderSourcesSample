// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UEGridComponent.generated.h"

enum class EUEGridLayer : uint8;
class FUEGridLayer;

UCLASS(Blueprintable, ClassGroup = (Custom), HideCategories = (Activation, Collision, Cooking, HLOD, Mobility, LOD, Navigation, Object, Physics))
class UNDEADEMPIRE_API UUEGridComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUEGridComponent();

	virtual void BeginPlay() override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual FBoxSphereBounds CalcBounds(FTransform const & LocalToWorld) const override;

	FIntPoint GetGridSize() const;
	FIntRect const & GetGridRect() const;
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

protected:
	void FillNatureObstacleLayer();
	FUEGridLayer & GetLayer(EUEGridLayer const GridLayer);
	FUEGridLayer const & GetLayer(EUEGridLayer const GridLayer) const;
	/** Converts coords to be used with UEGridLayer. */
	FUintPoint GetUnsignedCellCoordsUnsafe(FIntPoint const SignedCellCoords) const;
	FUintRect GetUnsignedRectUnsafe(FIntRect const & SignedRect) const;

private:
	double GetGridCellCornerZUnsafe(FIntPoint const Coords, UWorld const * const World, double const GridCellSize) const;

protected:
	TArray<FUEGridLayer> GridLayers;
	FIntRect GridRect;

	// Difference of cell corner Z from mean(of all 4 corners) for cell to be considered an obstacle.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UE|Obstacles Calculation")
	float ObstacleCellCornerZMinDifferenceFromMean;

	// Length of tracepath from 0. along positive and negative Z directions. Needed to fill nature obstacle layer.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UE|Obstacles Calculation")
	float GroundTraceHalfLength;

	// The channel used to fill nature obstacle layer.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UE|Obstacles Calculation")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel;
};
