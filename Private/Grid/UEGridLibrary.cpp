// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEGridLibrary.h"
#include "Common/UELog.h"
#include "Grid/UEGridSystem.h"

UUEGridSystem * UUEGridLibrary::GetGridSystem(UObject const * const WorldContextObject)
{
	UWorld * World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return UWorld::GetSubsystem<UUEGridSystem>(World);
}

float UUEGridLibrary::GetGridCellSize(UObject const * const WorldContextObject)
{
	if (UUEGridSystem * const GridSystem = GetGridSystem(WorldContextObject))
	{
		return GridSystem->GetCellSize();
	}
	return float{};
}

FVector2D UUEGridLibrary::GetGridCellCenter(UObject const * const WorldContextObject, FVector2D const Coords)
{
	double const GridCellSize = GetGridCellSize(WorldContextObject);
	double const CenterX = (Coords.X + 0.5) * GridCellSize;
	double const CenterY = (Coords.Y + 0.5) * GridCellSize;
	return FVector2D{ CenterX, CenterY };
}

FIntPoint UUEGridLibrary::GetGridCellCoords(UObject const * const WorldContextObject, FVector2D const Coords)
{
	if (UUEGridSystem * const GridSystem = GetGridSystem(WorldContextObject))
	{
		return GridSystem->GetCellCoords(Coords);
	}
	else
	{
		UE_LOGFMT(LogUE, Warning, "World of WorldContextObject passed to UUEGridLibrary::GetGridCellCoords has no UUEGridSystem.");
	}
	return FIntPoint{};
}

double UUEGridLibrary::GridSnap(UObject const * const WorldContextObject, double const InValue)
{
	return FMath::GridSnap(InValue, UUEGridLibrary::GetGridCellSize(WorldContextObject));
}

bool UUEGridLibrary::IsInSingleGridComponent(UObject const * const WorldContextObject, FIntRect const & Rect)
{
	if (UUEGridSystem * const GridSystem = GetGridSystem(WorldContextObject))
	{
		UUEGridComponent * const MinGridComponent = GridSystem->GetGridComponent(Rect.Min);
		return MinGridComponent != nullptr && MinGridComponent == GridSystem->GetGridComponent(Rect.Max - FIntPoint{ 1, 1 });
	}
	return false;
}

FIntRect UUEGridLibrary::GetGridIntRect(UObject const * const WorldContextObject, FBox2D const & Rect)
{
	if (UUEGridSystem * const GridSystem = GetGridSystem(WorldContextObject))
	{
		return GridSystem->GetIntRect(Rect);
	}
	else
	{
		UE_LOGFMT(LogUE, Warning, "World of WorldContextObject passed to UUEGridLibrary::GetGridIntRect has no UUEGridSystem.");
	}
	return FIntRect{};
}
