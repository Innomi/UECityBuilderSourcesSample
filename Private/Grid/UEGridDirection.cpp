// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEGridDirection.h"

TStaticArray<FIntPoint, FUEGridDirectionUtil::DirectionsNum> FUEGridDirectionUtil::GetAdjacentCoords(FIntPoint const Coords)
{
	TStaticArray<FIntPoint, DirectionsNum> AdjacentCoords;
	for (size_t CoordsIndex = 0; CoordsIndex < DirectionsNum; ++CoordsIndex)
	{
		AdjacentCoords[CoordsIndex] = Coords + AdjacentCoordsShifts[CoordsIndex];
	}
	return AdjacentCoords;
}

EUEGridDirection FUEGridDirectionUtil::GetDirection(FIntPoint const FromCoords, FIntPoint const ToCoords)
{
	if (FromCoords.X == ToCoords.X)
	{
		if (FromCoords.Y == ToCoords.Y)
		{
			return EUEGridDirection::NONE;
		}
		if (FromCoords.Y < ToCoords.Y)
		{
			return EUEGridDirection::East;
		}
		return EUEGridDirection::West;
	}
	else
	{
		if (FromCoords.Y != ToCoords.Y)
		{
			return EUEGridDirection::NONE;
		}
		if (FromCoords.X < ToCoords.X)
		{
			return EUEGridDirection::North;
		}
		return EUEGridDirection::South;
	}
}

FIntPoint const FUEGridDirectionUtil::AdjacentCoordsShifts[DirectionsNum] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };