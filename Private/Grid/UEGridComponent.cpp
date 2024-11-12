// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEGridComponent.h"
#include "Common/UELog.h"
#include "Grid/UEGridLayer.h"
#include "Grid/UEGridLibrary.h"
#include "Grid/UEGridSystem.h"

UUEGridComponent::UUEGridComponent()
{
	ObstacleCellCornerZMinDifferenceFromMean = 30.f;
	GroundTraceHalfLength = 9e4f;
	GroundTraceChannel = ECollisionChannel::ECC_WorldStatic;
}

void UUEGridComponent::BeginPlay()
{
	Super::BeginPlay();

	FillNatureObstacleLayer();
}

void UUEGridComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetComponentRotation().IsNearlyZero())
	{
		SetWorldRotation(FQuat::Identity);
		UE_LOGFMT(LogUE, Warning, "UUEGridComponent cannot have non-zero rotation, set to zero.");
	}

	FIntPoint const GridOrigin = UUEGridLibrary::GetGridCellCoords(this, FVector2D{ GetComponentLocation() });
	FIntPoint const GridSize = UUEGridLibrary::GetGridCellCoords(this, FVector2D{ GetComponentScale() });
	GridRect = FIntRect{ GridOrigin, GridOrigin + GridSize };
	for (EUEGridLayer const GridLayer : TEnumRange<EUEGridLayer>{})
	{
		GridLayers.Emplace(static_cast<FUintPoint>(GridSize));
	}

	if (UUEGridSystem * const GridSystem = UUEGridLibrary::GetGridSystem(this))
	{
		GridSystem->RegisterGridComponent(this);
	}
	else
	{
		UE_LOGFMT(LogUE, Warning, "No UUEGridSystem found in world, UUEGridComponent \"{0}\" cannot be registered.", GetName());
	}
}

void UUEGridComponent::OnUnregister()
{
	if (UUEGridSystem * const GridSystem = UUEGridLibrary::GetGridSystem(this))
	{
		GridSystem->UnregisterGridComponent(this);
	}

	GridLayers.Empty();
	GridRect = FIntRect{};

	Super::OnUnregister();
}

FBoxSphereBounds UUEGridComponent::CalcBounds(FTransform const & LocalToWorld) const
{
	return FBoxSphereBounds{ FBox{ FVector{ 0.f, 0.f, 0.f }, FVector{ 1.f, 1.f, 1.f } } }.TransformBy(LocalToWorld);
}

FIntPoint UUEGridComponent::GetGridSize() const
{
	return GridRect.Size();
}

FIntRect const & UUEGridComponent::GetGridRect() const
{
	return GridRect;
}

bool UUEGridComponent::IsInGrid(FIntPoint const CellCoords) const
{
	return GetGridRect().Contains(CellCoords);
}

bool UUEGridComponent::IsCellOccupied(EUEGridLayer const GridLayer, FVector2D const Coords) const
{
	return IsCellOccupied(GridLayer, UUEGridLibrary::GetGridCellCoords(this, Coords));
}

bool UUEGridComponent::IsCellOccupied(EUEGridLayer const GridLayer, FIntPoint const CellCoords) const
{
	if (!IsInGrid(CellCoords))
	{
		return false;
	}
	return GetLayer(GridLayer)[GetUnsignedCellCoordsUnsafe(CellCoords)];
}

bool UUEGridComponent::SetCellState(EUEGridLayer const GridLayer, FVector2D const Coords, bool const bIsOccupied)
{
	return SetCellState(GridLayer, UUEGridLibrary::GetGridCellCoords(this, Coords), bIsOccupied);
}

bool UUEGridComponent::SetCellState(EUEGridLayer const GridLayer, FIntPoint const CellCoords, bool const bIsOccupied)
{
	if (!IsInGrid(CellCoords))
	{
		return false;
	}
	GetLayer(GridLayer)[GetUnsignedCellCoordsUnsafe(CellCoords)] = bIsOccupied;
	return true;
}

bool UUEGridComponent::HasOccupiedCell(EUEGridLayer const GridLayer, FBox2D const & Rect) const
{
	return HasOccupiedCell(GridLayer, UUEGridLibrary::GetGridIntRect(this, Rect));
}

bool UUEGridComponent::HasOccupiedCell(EUEGridLayer const GridLayer, FIntRect const & Rect) const
{
	FIntRect ClippedRect = GetGridRect();
	ClippedRect.Clip(Rect);
	bool const bValue = true;
	return GetLayer(GridLayer).Contains(GetUnsignedRectUnsafe(ClippedRect), bValue);
}

void UUEGridComponent::SetCellsState(EUEGridLayer const GridLayer, FBox2D const & Rect, bool const bIsOccupied)
{
	SetCellsState(GridLayer, UUEGridLibrary::GetGridIntRect(this, Rect), bIsOccupied);
}

void UUEGridComponent::SetCellsState(EUEGridLayer const GridLayer, FIntRect const & Rect, bool const bIsOccupied)
{
	FIntRect ClippedRect = GetGridRect();
	ClippedRect.Clip(Rect);
	GetLayer(GridLayer).SetCells(GetUnsignedRectUnsafe(ClippedRect), bIsOccupied);
}

void UUEGridComponent::FillNatureObstacleLayer()
{
	check(GridLayers.Num() == static_cast<uint8>(EUEGridLayer::LAYERS_NUM));
	UWorld const * const World = GetWorld();
	if (UNLIKELY(!World) || GridRect.IsEmpty())
	{
		return;
	}
	float const GridCellSize = UUEGridLibrary::GetGridCellSize(this);
	TArray<float> PreviousXZs;
	PreviousXZs.Reserve(GridRect.Height() + 1);
	for (int32 Y = GridRect.Min.Y; Y <= GridRect.Max.Y; ++Y)
	{
		PreviousXZs.Emplace(GetGridCellCornerZUnsafe(FIntPoint{ GridRect.Min.X, Y }, World, GridCellSize));
	}
	for (int32 X = GridRect.Min.X + 1; X <= GridRect.Max.X; ++X)
	{
		float PreviousYZ = GetGridCellCornerZUnsafe(FIntPoint{ X, GridRect.Min.Y }, World, GridCellSize);
		for (int32 Y = GridRect.Min.Y + 1; Y <= GridRect.Max.Y; ++Y)
		{
			FIntPoint const CurrentCellCoords{ X, Y };
			float const CurrentYZ = GetGridCellCornerZUnsafe(CurrentCellCoords, World, GridCellSize);
			int32 const CurrentIndex = Y - GridRect.Min.Y;
			float const Mean = (CurrentYZ + PreviousYZ + PreviousXZs[CurrentIndex] + PreviousXZs[CurrentIndex - 1]) / 4.f;
			if (FMath::IsNearlyEqual(Mean, -GroundTraceHalfLength)
				|| Mean - CurrentYZ > ObstacleCellCornerZMinDifferenceFromMean
				|| Mean - PreviousYZ > ObstacleCellCornerZMinDifferenceFromMean
				|| Mean - PreviousXZs[CurrentIndex] > ObstacleCellCornerZMinDifferenceFromMean
				|| Mean - PreviousXZs[CurrentIndex - 1] > ObstacleCellCornerZMinDifferenceFromMean)
			{
				GridLayers[static_cast<uint8>(EUEGridLayer::NatureObstacle)][GetUnsignedCellCoordsUnsafe(CurrentCellCoords - FIntPoint{ 1, 1 })] = true;
			}
			PreviousXZs[CurrentIndex - 1] = PreviousYZ;
			PreviousYZ = CurrentYZ;
		}
		PreviousXZs[GridRect.Height()] = PreviousYZ;
	}
}

FUEGridLayer & UUEGridComponent::GetLayer(EUEGridLayer const GridLayer)
{
	return GridLayers[static_cast<uint8>(GridLayer)];
}

FUEGridLayer const & UUEGridComponent::GetLayer(EUEGridLayer const GridLayer) const
{
	return GridLayers[static_cast<uint8>(GridLayer)];
}

FUintPoint UUEGridComponent::GetUnsignedCellCoordsUnsafe(FIntPoint const SignedCellCoords) const
{
	return FUintPoint{ SignedCellCoords - GridRect.Min };
}

FUintRect UUEGridComponent::GetUnsignedRectUnsafe(FIntRect const & SignedRect) const
{
	return FUintRect{ GetUnsignedCellCoordsUnsafe(SignedRect.Min), GetUnsignedCellCoordsUnsafe(SignedRect.Max) };
}

double UUEGridComponent::GetGridCellCornerZUnsafe(FIntPoint const Coords, UWorld const * const World, double const GridCellSize) const
{
	FVector2D const CellCorner{ GridCellSize * Coords.X, GridCellSize * Coords.Y };
	FVector const Start{ CellCorner, GroundTraceHalfLength };
	FVector const End{ CellCorner, -GroundTraceHalfLength };
	FHitResult HitResult;
	if (World->LineTraceSingleByChannel(HitResult, Start, End, GroundTraceChannel))
	{
		return HitResult.Location.Z;
	}
	return End.Z;
}
