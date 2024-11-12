// Fill out your copyright notice in the Description page of Project Settings.

#include "Path/UEPathGraph.h"
#include "Grid/UEGridDirection.h"

FUEPathGraph::FVertexIndex const FUEPathGraph::NoConnection(-1);

void FUEPathGraph::AddVertex(FIntPoint const VertexCoords)
{
	if (UNLIKELY(IsVertex(VertexCoords)))
	{
		return;
	}
	CoordsToVertexIndex.Emplace(VertexCoords, Vertices.Num());
	VertexIndexToCoords.Emplace(Vertices.Num(), VertexCoords);
	Vertices.Emplace();
}

void FUEPathGraph::RemoveVertex(FIntPoint const VertexCoords)
{
	if (UNLIKELY(!IsVertex(VertexCoords)))
	{
		return;
	}
	FVertexIndex const VertexToRemoveIndex = CoordsToVertexIndex.FindAndRemoveChecked(VertexCoords);
	UpdateAdjacentVertices(VertexToRemoveIndex, NoConnection);
	Vertices.RemoveAtSwap(VertexToRemoveIndex, 1, false);
	FVertexIndex const LastVertexIndex = Vertices.Num();
	if (VertexToRemoveIndex == LastVertexIndex)
	{
		VertexIndexToCoords.Remove(VertexToRemoveIndex);
		return;
	}
	FIntPoint const LastVertexCoords = VertexIndexToCoords.FindAndRemoveChecked(LastVertexIndex);
	VertexIndexToCoords.Emplace(VertexToRemoveIndex, LastVertexCoords);
	CoordsToVertexIndex.Emplace(LastVertexCoords, VertexToRemoveIndex);
	UpdateAdjacentVertices(VertexToRemoveIndex, VertexToRemoveIndex);
}

void FUEPathGraph::ConnectVertices(FIntPoint const FirstVertexCoords, FIntPoint const SecondVertexCoords)
{
	FindMutualSideAndUpdateVertices(FirstVertexCoords, SecondVertexCoords, true);
}

void FUEPathGraph::DisconnectVertices(FIntPoint const FirstVertexCoords, FIntPoint const SecondVertexCoords)
{
	FindMutualSideAndUpdateVertices(FirstVertexCoords, SecondVertexCoords, false);
}

bool FUEPathGraph::AreConnected(FIntPoint const FirstVertexCoords, FIntPoint const SecondVertexCoords) const
{
	if (UNLIKELY(!((FirstVertexCoords.X == SecondVertexCoords.X || FirstVertexCoords.Y == SecondVertexCoords.Y)
		&& FirstVertexCoords != SecondVertexCoords
		&& IsVertex(FirstVertexCoords)
		&& IsVertex(SecondVertexCoords))))
	{
		return false;
	}
	FVertexIndex const FirstVertexIndex = CoordsToVertexIndex[FirstVertexCoords];
	FVertexIndex const SecondVertexIndex = CoordsToVertexIndex[SecondVertexCoords];
	FVertex const & FirstVertex = Vertices[FirstVertexIndex];
	EUEGridDirection const FirstVertexDirection = FUEGridDirectionUtil::GetDirection(FirstVertexCoords, SecondVertexCoords);
	if (FirstVertex.AdjacentVertices[static_cast<uint8>(FirstVertexDirection)] == SecondVertexIndex)
	{
		return true;
	}
	return false;
}

bool FUEPathGraph::IsVertex(FIntPoint const Coords) const
{
	return CoordsToVertexIndex.Contains(Coords);
}

FUEPathGraph::FVertex::FVertex()
{
	for (uint8 UIntPathDirection = 0; UIntPathDirection < static_cast<uint8>(EUEGridDirection::DIRECTIONS_NUM); ++UIntPathDirection)
	{
		AdjacentVertices[UIntPathDirection] = NoConnection;
	}
}

void FUEPathGraph::UpdateAdjacentVertices(FVertexIndex const VertexIndex, FVertexIndex const IndexToUpdateTo)
{
	for (uint8 UIntPathDirection = 0; UIntPathDirection < static_cast<uint8>(EUEGridDirection::DIRECTIONS_NUM); ++UIntPathDirection)
	{
		FVertexIndex const AdjacentVertexIndex = Vertices[VertexIndex].AdjacentVertices[UIntPathDirection];
		if (AdjacentVertexIndex != NoConnection)
		{
			EUEGridDirection const OppositePathDirection = FUEGridDirectionUtil::GetOppositeDirectionUnsafe(static_cast<EUEGridDirection>(UIntPathDirection));
			Vertices[AdjacentVertexIndex].AdjacentVertices[static_cast<uint8>(OppositePathDirection)] = IndexToUpdateTo;
		}
	}
}

void FUEPathGraph::FindMutualSideAndUpdateVertices(FIntPoint const FirstVertexCoords, FIntPoint const SecondVertexCoords, bool const bConnect)
{
	if (UNLIKELY(!((FirstVertexCoords.X == SecondVertexCoords.X || FirstVertexCoords.Y == SecondVertexCoords.Y)
		&& FirstVertexCoords != SecondVertexCoords
		&& IsVertex(FirstVertexCoords)
		&& IsVertex(SecondVertexCoords))))
	{
		return;
	}
	FVertexIndex const FirstVertexIndex = CoordsToVertexIndex[FirstVertexCoords];
	FVertexIndex const SecondVertexIndex = CoordsToVertexIndex[SecondVertexCoords];
	EUEGridDirection const FirstVertexDirection = FUEGridDirectionUtil::GetDirection(FirstVertexCoords, SecondVertexCoords);
	EUEGridDirection const SecondVertexDirection = FUEGridDirectionUtil::GetOppositeDirectionUnsafe(FirstVertexDirection);
	FVertexIndex & FirstVertexConnection = Vertices[FirstVertexIndex].AdjacentVertices[static_cast<uint8>(FirstVertexDirection)];
	FVertexIndex & SecondVertexConnection = Vertices[SecondVertexIndex].AdjacentVertices[static_cast<uint8>(SecondVertexDirection)];
	if (bConnect)
	{
		if (FirstVertexConnection != NoConnection)
		{
			Vertices[FirstVertexConnection].AdjacentVertices[static_cast<uint8>(SecondVertexDirection)] = NoConnection;
		}
		if (SecondVertexConnection != NoConnection)
		{
			Vertices[SecondVertexConnection].AdjacentVertices[static_cast<uint8>(FirstVertexDirection)] = NoConnection;
		}
		FirstVertexConnection = SecondVertexIndex;
		SecondVertexConnection = FirstVertexIndex;
	}
	else
	{
		if (FirstVertexConnection != SecondVertexIndex)
		{
			return;
		}
		FirstVertexConnection = NoConnection;
		SecondVertexConnection = NoConnection;
	}
}