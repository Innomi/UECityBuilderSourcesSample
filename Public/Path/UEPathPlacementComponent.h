// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grid/UEGridPlacementComponent.h"
#include "UEPathPlacementComponent.generated.h"

enum class EUEGridDirection : uint8;
enum class EUEGridLayer : uint8;
enum class EUEPathGraph : uint8;
class UUEPathSystem;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNDEADEMPIRE_API UUEPathPlacementComponent : public UUEGridPlacementComponent
{
	GENERATED_BODY()

public:
	UUEPathPlacementComponent();

	void RegisterPath(FIntPoint const FromCoords, FIntPoint const ToCoords) const;
	void UnregisterPath(FIntRect const & Rect) const;
	TObjectPtr<UUEPathSystem> GetPathSystem() const;
	bool IsPathAt(FIntPoint const Coords) const;
	bool ShouldBeVertex(FIntPoint const Coords) const;
	EUEGridLayer GetPathRelatedGridLayer() const;
	EUEPathGraph GetPathGraphToRegister() const;

protected:
	bool AreAdjacentCellsCorrespondPattern(FIntPoint const Coords, bool const Pattern[4]) const;

	UPROPERTY(EditDefaultsOnly)
	EUEPathGraph PathGraphToRegister;

	UPROPERTY(EditDefaultsOnly)
	EUEGridLayer PathRelatedGridLayerToRegisterOn;

private:
	void GatherAdjacentVerticesUpdatesAfterPathRegistration(FIntRect const & NewPathRect, TArray<FIntPoint> & OutVerticesToAdd, TArray<FIntPoint> & OutVerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnectionsToAdd) const;
	void CheckAdjacentCellToUpdateVertexAfterPathRegistration(FIntPoint const Coords, EUEGridDirection const NewPathToCellDirection, TArray<FIntPoint> & OutVerticesToAdd, TArray<FIntPoint> & OutVerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnectionsToAdd) const;
	void GatherAdjacentVerticesUpdatesBeforePathUnregistration(FIntRect const & UnregistrationRect, TArray<FIntPoint> & OutVerticesToAdd, TArray<FIntPoint> & OutVerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnectionsToRemove) const;
	void CheckAdjacentCellToUpdateVertexBeforePathUnregistration(FIntPoint const Coords, EUEGridDirection const UnregisteredPathToCellDirection, TArray<FIntPoint> & OutVerticesToAdd, TArray<FIntPoint> & OutVerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnectionsToRemove) const;
	FIntPoint GetFirstMetVertexCoords(FIntPoint Coords, FIntPoint const Shift) const;
	void GetConnectionOverPathCell(FIntPoint const Coords, EUEGridDirection const GridDirection, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnections) const;
	void GetVertexConnection(FIntPoint const VertexCoords, EUEGridDirection const GridDirection, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnections) const;
	void GetVertexConnections(TArray<FIntPoint> const & Vertices, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnections) const;
	void GetVertexConnections(FIntPoint const VertexCoords, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnections) const;
	void GetVertexCoordsFromRect(FIntRect const & Rect, TArray<FIntPoint> & OutVertexCoords) const;
};
