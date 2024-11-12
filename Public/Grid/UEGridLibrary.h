// Fill out your copyright notice in the Description page of Project Settings.

#pragma once 

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UEGridLibrary.generated.h"

/**
 * UUEGridLibrary
 */
UCLASS(meta = (ScriptName = "UEGridLibrary"))
class UNDEADEMPIRE_API UUEGridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "UEGrid", meta = (WorldContext = "WorldContextObject"))
	static UUEGridSystem * GetGridSystem(UObject const * const WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "UEGrid", meta = (WorldContext = "WorldContextObject"))
	static float GetGridCellSize(UObject const * const WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "UEGrid", meta = (WorldContext = "WorldContextObject"))
	static FVector2D GetGridCellCenter(UObject const * const WorldContextObject, FVector2D const Coords);

	UFUNCTION(BlueprintPure, Category = "UEGrid", meta = (WorldContext = "WorldContextObject"))
	static FIntPoint GetGridCellCoords(UObject const * const WorldContextObject, FVector2D const Coords);

	UFUNCTION(BlueprintPure, Category = "UEGrid", meta = (WorldContext = "WorldContextObject"))
	static double GridSnap(UObject const * const WorldContextObject, double const InValue);

	static bool IsInSingleGridComponent(UObject const * const WorldContextObject, FIntRect const & Rect);

	static FIntRect GetGridIntRect(UObject const * const WorldContextObject, FBox2D const & Rect);
};
