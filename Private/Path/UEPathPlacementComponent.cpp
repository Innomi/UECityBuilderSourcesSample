// Fill out your copyright notice in the Description page of Project Settings.

#include "Path/UEPathPlacementComponent.h"
#include "Grid/UEGridDirection.h"
#include "Grid/UEGridLibrary.h"
#include "Grid/UEGridSystem.h"
#include "Path/UEPathGraph.h"
#include "Path/UEPathSystem.h"

namespace
{
	FIntRect const GetAdjacentRect(FIntRect const & Rect, EUEGridDirection const GridDirection)
	{
		switch (GridDirection)
		{
			case EUEGridDirection::North:
			{
				return FIntRect{ Rect.Min + FIntPoint{ Rect.Width(), 0 }, Rect.Max + FIntPoint{ 1, 0 } };
			}
			case EUEGridDirection::East:
			{
				return FIntRect{ Rect.Min + FIntPoint{ 0, Rect.Height() }, Rect.Max + FIntPoint{ 0, 1 } };
			}
			case EUEGridDirection::South:
			{
				return FIntRect{ Rect.Min - FIntPoint{ 1, 0 }, Rect.Max - FIntPoint{ Rect.Width(), 0 } };
			}
			case EUEGridDirection::West:
			{
				return FIntRect{ Rect.Min - FIntPoint{ 0, 1 }, Rect.Max - FIntPoint{ 0, Rect.Height() } };
			}
			default:
			{
				checkNoEntry();
				return FIntRect{};
			}
		}
	}
} // namespace

UUEPathPlacementComponent::UUEPathPlacementComponent()
{
	SetShouldBeRegisteredOnGrid(false);
	SetGridSize(FIntPoint{ 1, 1 });
	PathGraphToRegister = EUEPathGraph::Road;
	PathRelatedGridLayerToRegisterOn = EUEGridLayer::Road;
}

void UUEPathPlacementComponent::RegisterPath(FIntPoint const FromCoords, FIntPoint const ToCoords) const
{
	if (FromCoords.X != ToCoords.X && FromCoords.Y != ToCoords.Y)
	{
		return;
	}
	TObjectPtr<UUEPathSystem> const PathSystem = GetPathSystem();
	if (UNLIKELY(!IsValid(PathSystem)))
	{
		return;
	}
	
	FIntPoint const Min = FromCoords.ComponentMin(ToCoords);
	FIntPoint const Max = FromCoords.ComponentMax(ToCoords);
	FIntRect const PathRect{ Min, Max + FIntPoint{ 1, 1 } };
	if (!CanBePlacedOnGrid(PathRect))
	{
		return;
	}

	bool const bIsOccupied = true;
	SetOccupationOnGrid(GridLayerToRegisterOn, PathRect, bIsOccupied);
	SetOccupationOnGrid(PathRelatedGridLayerToRegisterOn, PathRect, bIsOccupied);

	TArray<FIntPoint> VerticesToAdd;
	TArray<FIntPoint> VerticesToRemove;
	TArray<TTuple<FIntPoint, FIntPoint>> ConnectionsToAdd;

	GetVertexCoordsFromRect(PathRect, VerticesToAdd);
	GetVertexConnections(VerticesToAdd, ConnectionsToAdd);
	GatherAdjacentVerticesUpdatesAfterPathRegistration(PathRect, VerticesToAdd, VerticesToRemove, ConnectionsToAdd);

	PathSystem->UpdateGraphAsync(PathGraphToRegister, MoveTemp(VerticesToAdd), MoveTemp(VerticesToRemove), MoveTemp(ConnectionsToAdd), TArray<TTuple<FIntPoint, FIntPoint>>{});
}

void UUEPathPlacementComponent::UnregisterPath(FIntRect const & Rect) const
{
	if (!ensure(Rect.Min.X <= Rect.Max.X && Rect.Min.Y <= Rect.Max.Y) || Rect.IsEmpty() || !UUEGridLibrary::IsInSingleGridComponent(this, Rect))
	{
		return;
	}

	TObjectPtr<UUEPathSystem> const PathSystem = GetPathSystem();
	TObjectPtr<UUEGridSystem> const GridSystem = UUEGridLibrary::GetGridSystem(this);
	if (UNLIKELY(!IsValid(GridSystem) || !IsValid(PathSystem)))
	{
		return;
	}

	TArray<FIntPoint> VerticesToAdd;
	TArray<FIntPoint> VerticesToRemove;
	TArray<TTuple<FIntPoint, FIntPoint>> ConnectionsToAdd;
	TArray<TTuple<FIntPoint, FIntPoint>> ConnectionsToRemove;

	GatherAdjacentVerticesUpdatesBeforePathUnregistration(Rect, VerticesToAdd, VerticesToRemove, ConnectionsToRemove);
	int32 const AdjacentVerticesToRemoveNum = VerticesToRemove.Num();

	for (int32 X = Rect.Min.X; X < Rect.Max.X; ++X)
	{
		for (int32 Y = Rect.Min.Y; Y < Rect.Max.Y; ++Y)
		{
			FIntPoint const CurrentCoords{ X, Y };
			if (GridSystem->IsCellOccupied(PathRelatedGridLayerToRegisterOn, CurrentCoords))
			{
				if (ShouldBeVertex(CurrentCoords))
				{
					VerticesToRemove.Emplace(CurrentCoords);
				}
				bool const bIsOccupied = false;
				SetOccupationOnGrid(GridLayerToRegisterOn, CurrentCoords, bIsOccupied);
			}
		}
	}

	bool const bIsOccupied = false;
	SetOccupationOnGrid(PathRelatedGridLayerToRegisterOn, Rect, bIsOccupied);

	for (int32 AdjacentVertexToRemoveIndex = 0; AdjacentVertexToRemoveIndex < AdjacentVerticesToRemoveNum; ++AdjacentVertexToRemoveIndex)
	{
		GetConnectionOverPathCell(VerticesToRemove[AdjacentVertexToRemoveIndex], EUEGridDirection::North, ConnectionsToAdd);
		GetConnectionOverPathCell(VerticesToRemove[AdjacentVertexToRemoveIndex], EUEGridDirection::East, ConnectionsToAdd);
	}
	GetVertexConnections(VerticesToAdd, ConnectionsToAdd);

	PathSystem->UpdateGraphAsync(PathGraphToRegister, MoveTemp(VerticesToAdd), MoveTemp(VerticesToRemove), MoveTemp(ConnectionsToAdd), MoveTemp(ConnectionsToRemove));
}

TObjectPtr<UUEPathSystem> UUEPathPlacementComponent::GetPathSystem() const
{
	if (TObjectPtr<UWorld> const World = GetWorld())
	{
		return World->GetSubsystem<UUEPathSystem>();
	}
	return nullptr;
}

bool UUEPathPlacementComponent::IsPathAt(FIntPoint const Coords) const
{
	TObjectPtr<UUEGridSystem> const GridSystem = UUEGridLibrary::GetGridSystem(this);
	if (UNLIKELY(!IsValid(GridSystem)))
	{
		return false;
	}
	return GridSystem->IsCellOccupied(PathRelatedGridLayerToRegisterOn, Coords);
}

bool UUEPathPlacementComponent::ShouldBeVertex(FIntPoint const Coords) const
{
	if (!IsPathAt(Coords))
	{
		return false;
	}
	bool const NotVertexPatternX[4] = { true, false, true, false };
	bool const NotVertexPatternY[4] = { false, true, false, true };
	return !(AreAdjacentCellsCorrespondPattern(Coords, NotVertexPatternX) || AreAdjacentCellsCorrespondPattern(Coords, NotVertexPatternY));
}

EUEGridLayer UUEPathPlacementComponent::GetPathRelatedGridLayer() const
{
	return PathRelatedGridLayerToRegisterOn;
}

EUEPathGraph UUEPathPlacementComponent::GetPathGraphToRegister() const
{
	return PathGraphToRegister;
}

bool UUEPathPlacementComponent::AreAdjacentCellsCorrespondPattern(FIntPoint const Coords, bool const Pattern[4]) const
{
	TObjectPtr<UUEGridSystem> const GridSystem = UUEGridLibrary::GetGridSystem(this);
	if (UNLIKELY(!IsValid(GridSystem)))
	{
		return false;
	}
	for (EUEGridDirection const GridDirection : TEnumRange<EUEGridDirection>())
	{
		FIntPoint const AdjacentCellCoords = FUEGridDirectionUtil::GetAdjacentCoordsUnsafe(Coords, GridDirection);
		uint8 const UintGridDirection = static_cast<uint8>(GridDirection);
		if (GridSystem->IsCellOccupied(PathRelatedGridLayerToRegisterOn, AdjacentCellCoords) != Pattern[UintGridDirection])
		{
			return false;
		}
	}
	return true;
}

void UUEPathPlacementComponent::GatherAdjacentVerticesUpdatesAfterPathRegistration(FIntRect const & NewPathRect, TArray<FIntPoint> & OutVerticesToAdd, TArray<FIntPoint> & OutVerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnectionsToAdd) const
{
	for (EUEGridDirection const GridDirection : TEnumRange<EUEGridDirection>())
	{
		FIntRect const AdjacentToNewPathRect = GetAdjacentRect(NewPathRect, GridDirection);
		for (int32 X = AdjacentToNewPathRect.Min.X; X < AdjacentToNewPathRect.Max.X; ++X)
		{
			for (int32 Y = AdjacentToNewPathRect.Min.Y; Y < AdjacentToNewPathRect.Max.Y; ++Y)
			{
				FIntPoint const CurrentCoords{ X, Y };
				CheckAdjacentCellToUpdateVertexAfterPathRegistration(CurrentCoords, GridDirection, OutVerticesToAdd, OutVerticesToRemove, OutConnectionsToAdd);
			}
		}
	}
}

void UUEPathPlacementComponent::CheckAdjacentCellToUpdateVertexAfterPathRegistration(FIntPoint const Coords, EUEGridDirection const NewPathToCellDirection, TArray<FIntPoint> & OutVerticesToAdd, TArray<FIntPoint> & OutVerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnectionsToAdd) const
{
	check(static_cast<uint8>(NewPathToCellDirection) < static_cast<uint8>(EUEGridDirection::DIRECTIONS_NUM));
	if (!IsPathAt(Coords))
	{
		return;
	}
	bool PatternToCheckForAddition[4]{ true, true, true, true };
	bool PatternToCheckForRemoval[4]{};
	EUEGridDirection const CellToNewPathDirection = FUEGridDirectionUtil::GetOppositeDirectionUnsafe(NewPathToCellDirection);
	uint8 const UintNewPathToCellDirection = static_cast<uint8>(NewPathToCellDirection);
	uint8 const UintCellToNewPathDirection = static_cast<uint8>(CellToNewPathDirection);
	PatternToCheckForAddition[UintNewPathToCellDirection] = false;
	PatternToCheckForRemoval[UintCellToNewPathDirection] = true;
	PatternToCheckForRemoval[UintNewPathToCellDirection] = true;
	if (AreAdjacentCellsCorrespondPattern(Coords, PatternToCheckForAddition))
	{
		OutVerticesToAdd.Emplace(Coords);
		GetVertexConnection(Coords, CellToNewPathDirection, OutConnectionsToAdd);
		GetVertexConnection(Coords, FUEGridDirectionUtil::GetCWNextDirectionUnsafe(CellToNewPathDirection), OutConnectionsToAdd);
		GetVertexConnection(Coords, FUEGridDirectionUtil::GetCCWNextDirectionUnsafe(CellToNewPathDirection), OutConnectionsToAdd);
	}
	else if (AreAdjacentCellsCorrespondPattern(Coords, PatternToCheckForRemoval))
	{
		OutVerticesToRemove.Emplace(Coords);
		GetConnectionOverPathCell(Coords, CellToNewPathDirection, OutConnectionsToAdd);
	}
	else
	{
		GetVertexConnection(Coords, CellToNewPathDirection, OutConnectionsToAdd);
	}
}

void UUEPathPlacementComponent::GatherAdjacentVerticesUpdatesBeforePathUnregistration(FIntRect const & UnregistrationRect, TArray<FIntPoint> & OutVerticesToAdd, TArray<FIntPoint> & OutVerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnectionsToRemove) const
{
	for (EUEGridDirection const GridDirection : TEnumRange<EUEGridDirection>())
	{
		EUEGridDirection const OppositeGridDirection = FUEGridDirectionUtil::GetOppositeDirectionUnsafe(GridDirection);
		FIntRect const AdjacentToUnregisteredPathRect = GetAdjacentRect(UnregistrationRect, GridDirection);
		for (int32 X = AdjacentToUnregisteredPathRect.Min.X; X < AdjacentToUnregisteredPathRect.Max.X; ++X)
		{
			for (int32 Y = AdjacentToUnregisteredPathRect.Min.Y; Y < AdjacentToUnregisteredPathRect.Max.Y; ++Y)
			{
				FIntPoint const CurrentCoords{ X, Y };
				FIntPoint const AdjacentUnregisteredCell = FUEGridDirectionUtil::GetAdjacentCoordsUnsafe(CurrentCoords, OppositeGridDirection);
				if (IsPathAt(AdjacentUnregisteredCell))
				{
					CheckAdjacentCellToUpdateVertexBeforePathUnregistration(CurrentCoords, GridDirection, OutVerticesToAdd, OutVerticesToRemove, OutConnectionsToRemove);
				}
			}
		}
	}
}

void UUEPathPlacementComponent::CheckAdjacentCellToUpdateVertexBeforePathUnregistration(FIntPoint const Coords, EUEGridDirection const UnregisteredPathToCellDirection, TArray<FIntPoint> & OutVerticesToAdd, TArray<FIntPoint> & OutVerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnectionsToRemove) const
{
	check(static_cast<uint8>(UnregisteredPathToCellDirection) < static_cast<uint8>(EUEGridDirection::DIRECTIONS_NUM));
	if (!IsPathAt(Coords))
	{
		return;
	}
	bool PatternToCheckForAddition[4]{};
	bool PatternToCheckForRemoval[4]{ true, true, true, true };
	EUEGridDirection const CellToUnregisteredPathDirection = FUEGridDirectionUtil::GetOppositeDirectionUnsafe(UnregisteredPathToCellDirection);
	uint8 const UintUnregisteredPathToCellDirection = static_cast<uint8>(UnregisteredPathToCellDirection);
	uint8 const UintCellToUnregisteredPathDirection = static_cast<uint8>(CellToUnregisteredPathDirection);
	PatternToCheckForRemoval[UintUnregisteredPathToCellDirection] = false;
	PatternToCheckForAddition[UintCellToUnregisteredPathDirection] = true;
	PatternToCheckForAddition[UintUnregisteredPathToCellDirection] = true;
	if (AreAdjacentCellsCorrespondPattern(Coords, PatternToCheckForAddition))
	{
		OutVerticesToAdd.Emplace(Coords);
	}
	else if (AreAdjacentCellsCorrespondPattern(Coords, PatternToCheckForRemoval))
	{
		OutVerticesToRemove.Emplace(Coords);
	}
	else
	{
		GetVertexConnection(Coords, CellToUnregisteredPathDirection, OutConnectionsToRemove);
	}
}

FIntPoint UUEPathPlacementComponent::GetFirstMetVertexCoords(FIntPoint Coords, FIntPoint const Shift) const
{
	check(IsPathAt(Coords) && Shift != FIntPoint::ZeroValue);
	while (!ShouldBeVertex(Coords))
	{
		Coords += Shift;
	}
	return Coords;
}

void UUEPathPlacementComponent::GetConnectionOverPathCell(FIntPoint const Coords, EUEGridDirection const GridDirection, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnections) const
{
	check(IsPathAt(Coords) && !ShouldBeVertex(Coords));
	FIntPoint const Shift = FUEGridDirectionUtil::GetAdjacentCoordsShiftUnsafe(GridDirection);
	if (!IsPathAt(Coords + Shift) || !IsPathAt(Coords - Shift))
	{
		return;
	}
	FIntPoint const ForthVertexCoords = GetFirstMetVertexCoords(Coords + Shift, Shift);
	FIntPoint const BackVertexCoords = GetFirstMetVertexCoords(Coords - Shift, Shift * -1);
	OutConnections.Emplace(ForthVertexCoords, BackVertexCoords);
}

void UUEPathPlacementComponent::GetVertexConnection(FIntPoint const VertexCoords, EUEGridDirection const GridDirection, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnections) const
{
	check(IsPathAt(VertexCoords));
	FIntPoint Shift = FUEGridDirectionUtil::GetAdjacentCoordsShiftUnsafe(GridDirection);
	if (IsPathAt(VertexCoords + Shift))
	{
		FIntPoint const SecondVertexCoords = GetFirstMetVertexCoords(VertexCoords + Shift, Shift);
		OutConnections.Emplace(VertexCoords, SecondVertexCoords);
	}
}

void UUEPathPlacementComponent::GetVertexConnections(TArray<FIntPoint> const & Vertices, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnections) const
{
	for (FIntPoint const Vertex : Vertices)
	{
		GetVertexConnections(Vertex, OutConnections);
	}
}

void UUEPathPlacementComponent::GetVertexConnections(FIntPoint const VertexCoords, TArray<TTuple<FIntPoint, FIntPoint>> & OutConnections) const
{
	for (EUEGridDirection const CurrentDirection : TEnumRange<EUEGridDirection>())
	{
		GetVertexConnection(VertexCoords, CurrentDirection, OutConnections);
	}
}

void UUEPathPlacementComponent::GetVertexCoordsFromRect(FIntRect const & Rect, TArray<FIntPoint> & OutVertexCoords) const
{
	for (int32 X = Rect.Min.X; X < Rect.Max.X; ++X)
	{
		for (int32 Y = Rect.Min.Y; Y < Rect.Max.Y; ++Y)
		{
			FIntPoint const CurrentCoords{ X, Y };
			if (ShouldBeVertex(CurrentCoords))
			{
				OutVertexCoords.Emplace(CurrentCoords);
			}
		}
	}
}