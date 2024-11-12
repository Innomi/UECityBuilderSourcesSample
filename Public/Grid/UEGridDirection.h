// Fill out your copyright notice in the Description page of Project Settings.

#pragma once 

#include "CoreMinimal.h"

UENUM()
enum class EUEGridDirection : uint8
{
	North,
	East,
	South,
	West,
	DIRECTIONS_NUM UMETA(Hidden),
	NONE
};

ENUM_RANGE_BY_COUNT(EUEGridDirection, EUEGridDirection::DIRECTIONS_NUM)

/**
 * FUEGridDirectionUtil
 */
class UNDEADEMPIRE_API FUEGridDirectionUtil
{
public:
	static constexpr uint8 DirectionsNum = static_cast<uint8>(EUEGridDirection::DIRECTIONS_NUM);

	static FORCEINLINE FIntPoint GetAdjacentCoordsUnsafe(FIntPoint const Coords, EUEGridDirection const GridDirection);
	static TStaticArray<FIntPoint, DirectionsNum> GetAdjacentCoords(FIntPoint const Coords);
	static FORCEINLINE FIntPoint GetAdjacentCoordsShiftUnsafe(EUEGridDirection const GridDirection);
	static FORCEINLINE FIntPoint const (& GetAdjacentCoordsShifts())[DirectionsNum];
	static FORCEINLINE EUEGridDirection GetCCWNextDirectionUnsafe(EUEGridDirection const GridDirection);
	static FORCEINLINE EUEGridDirection GetCWNextDirectionUnsafe(EUEGridDirection const GridDirection);
	static FORCEINLINE EUEGridDirection GetOppositeDirectionUnsafe(EUEGridDirection const GridDirection);
	static EUEGridDirection GetDirection(FIntPoint const FromCoords, FIntPoint const ToCoords);

private:
	static FIntPoint const AdjacentCoordsShifts[DirectionsNum];
};

FIntPoint FUEGridDirectionUtil::GetAdjacentCoordsUnsafe(FIntPoint const Coords, EUEGridDirection const GridDirection)
{
	check(static_cast<uint8>(GridDirection) < DirectionsNum);
	return Coords + AdjacentCoordsShifts[static_cast<uint8>(GridDirection)];
}

FIntPoint FUEGridDirectionUtil::GetAdjacentCoordsShiftUnsafe(EUEGridDirection const GridDirection)
{
	check(static_cast<uint8>(GridDirection) < DirectionsNum);
	return AdjacentCoordsShifts[static_cast<uint8>(GridDirection)];
}

FIntPoint const (& FUEGridDirectionUtil::GetAdjacentCoordsShifts())[DirectionsNum]
{
	return AdjacentCoordsShifts;
}

EUEGridDirection FUEGridDirectionUtil::GetCCWNextDirectionUnsafe(EUEGridDirection const GridDirection)
{
	check(static_cast<uint8>(GridDirection) < DirectionsNum);
	return static_cast<EUEGridDirection>((static_cast<uint8>(GridDirection) + 3) % DirectionsNum);
}

EUEGridDirection FUEGridDirectionUtil::GetCWNextDirectionUnsafe(EUEGridDirection const GridDirection)
{
	check(static_cast<uint8>(GridDirection) < DirectionsNum);
	return static_cast<EUEGridDirection>((static_cast<uint8>(GridDirection) + 1) % DirectionsNum);
}

EUEGridDirection FUEGridDirectionUtil::GetOppositeDirectionUnsafe(EUEGridDirection const GridDirection)
{
	check(static_cast<uint8>(GridDirection) < DirectionsNum);
	return static_cast<EUEGridDirection>((static_cast<uint8>(GridDirection) + 2) % DirectionsNum);
}