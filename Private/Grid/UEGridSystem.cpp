// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEGridSystem.h"
#include "Common/UELog.h"
#include "Grid/UEGridComponent.h"

UUEGridSystem::UUEGridSystem()
	: CellSize(100.f)
{
}

void UUEGridSystem::RegisterGridComponent(TObjectPtr<UUEGridComponent> const GridComponent)
{
	if (!IsValid(GridComponent))
	{
		return;
	}

	check(GridComponents.Num() == GridRects.Num());
	FIntRect const GridRect = GridComponent->GetGridRect();
	for (size_t ComponentIndex = 0; ComponentIndex < GridComponents.Num(); ++ComponentIndex)
	{
		if (GridComponents[ComponentIndex] == GridComponent)
		{
			return;
		}
		if (GridRects[ComponentIndex].Intersect(GridRect))
		{
			UE_LOGFMT(LogUE, Warning, "UUEGridComponent \"{0}\" overlaps with already placed grid, it cannot be registered.", GridComponent->GetName());
			return;
		}
	}
	GridComponents.Emplace(GridComponent);
	GridRects.Emplace(GridRect);
}

void UUEGridSystem::UnregisterGridComponent(TObjectPtr<UUEGridComponent> const GridComponent)
{
	if (!IsValid(GridComponent))
	{
		return;
	}

	if (size_t const ComponentIndex = GridComponents.Find(GridComponent); ComponentIndex != INDEX_NONE)
	{
		GridComponents.RemoveAtSwap(ComponentIndex);
		GridRects.RemoveAtSwap(ComponentIndex);
	}
}

TObjectPtr<UUEGridComponent> UUEGridSystem::GetGridComponent(FVector2D const Coords) const
{
	return GetGridComponent(GetCellCoords(Coords));
}

TObjectPtr<UUEGridComponent> UUEGridSystem::GetGridComponent(FIntPoint const Coords) const
{
	check(GridComponents.Num() == GridRects.Num());
	for (size_t ComponentIndex = 0; ComponentIndex < GridComponents.Num(); ++ComponentIndex)
	{
		if (GridRects[ComponentIndex].Contains(Coords))
		{
			return GridComponents[ComponentIndex];
		}
	}
	return nullptr;
}

TArray<TObjectPtr<UUEGridComponent>> UUEGridSystem::GetGridComponents(FBox2D const & Rect) const
{
	return GetGridComponents(GetIntRect(Rect));
}

TArray<TObjectPtr<UUEGridComponent>> UUEGridSystem::GetGridComponents(FIntRect const Rect) const
{
	check(GridComponents.Num() == GridRects.Num());
	TArray<TObjectPtr<UUEGridComponent>> Components;
	for (size_t ComponentIndex = 0; ComponentIndex < GridComponents.Num(); ++ComponentIndex)
	{
		if (GridRects[ComponentIndex].Intersect(Rect))
		{
			Components.Emplace(GridComponents[ComponentIndex]);
		}
	}
	return Components;
}

float UUEGridSystem::GetCellSize() const
{
	return CellSize;
}

FIntPoint UUEGridSystem::GetCellCoords(FVector2D const Coords) const
{
	int32 const X = FMath::RoundToNegativeInfinity(Coords.X / CellSize);
	int32 const Y = FMath::RoundToNegativeInfinity(Coords.Y / CellSize);
	return FIntPoint(X, Y);
}

FIntRect UUEGridSystem::GetIntRect(FBox2D const & Rect) const
{
	return FIntRect(GetCellCoords(Rect.Min), GetCellCoords(Rect.Max));
}

bool UUEGridSystem::IsInGrid(FIntPoint const CellCoords) const
{
	for (FIntRect const & Rect : GridRects)
	{
		if (Rect.Contains(CellCoords))
		{
			return true;
		}
	}
	return false;
}

bool UUEGridSystem::IsCellOccupied(EUEGridLayer const GridLayer, FVector2D const Coords) const
{
	return IsCellOccupied(GridLayer, GetCellCoords(Coords));
}

bool UUEGridSystem::IsCellOccupied(EUEGridLayer const GridLayer, FIntPoint const CellCoords) const
{
	if (TObjectPtr<UUEGridComponent> const GridComponent = GetGridComponent(CellCoords))
	{
		return GridComponent->IsCellOccupied(GridLayer, CellCoords);
	}
	return false;
}

bool UUEGridSystem::SetCellState(EUEGridLayer const GridLayer, FVector2D const Coords, bool const bIsOccupied)
{
	return SetCellState(GridLayer, GetCellCoords(Coords), bIsOccupied);
}

bool UUEGridSystem::SetCellState(EUEGridLayer const GridLayer, FIntPoint const CellCoords, bool const bIsOccupied)
{
	if (TObjectPtr<UUEGridComponent> const GridComponent = GetGridComponent(CellCoords))
	{
		return GridComponent->SetCellState(GridLayer, CellCoords, bIsOccupied);
	}
	return false;
}

bool UUEGridSystem::HasOccupiedCell(EUEGridLayer const GridLayer, FBox2D const & Rect) const
{
	return HasOccupiedCell(GridLayer, GetIntRect(Rect));
}

bool UUEGridSystem::HasOccupiedCell(EUEGridLayer const GridLayer, FIntRect const & Rect) const
{
	for (TObjectPtr<UUEGridComponent> const GridComponent : GetGridComponents(Rect))
	{
		check(GridComponent);
		if (GridComponent->HasOccupiedCell(GridLayer, Rect))
		{
			return true;
		}
	}
	return false;
}

void UUEGridSystem::SetCellsState(EUEGridLayer const GridLayer, FBox2D const & Rect, bool const bIsOccupied)
{
	SetCellsState(GridLayer, GetIntRect(Rect), bIsOccupied);
}

void UUEGridSystem::SetCellsState(EUEGridLayer const GridLayer, FIntRect const & Rect, bool const bIsOccupied)
{
	for (TObjectPtr<UUEGridComponent> const GridComponent : GetGridComponents(Rect))
	{
		check(GridComponent);
		GridComponent->SetCellsState(GridLayer, Rect, bIsOccupied);
	}
}
