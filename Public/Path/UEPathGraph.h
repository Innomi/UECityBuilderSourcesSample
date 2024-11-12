// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UNDEADEMPIRE_API FUEPathGraph
{
public:
	void AddVertex(FIntPoint const VertexCoords);
	void RemoveVertex(FIntPoint const VertexCoords);
	void ConnectVertices(FIntPoint const FirstVertexCoords, FIntPoint const SecondVertexCoords);
	void DisconnectVertices(FIntPoint const FirstVertexCoords, FIntPoint const SecondVertexCoords);
	bool AreConnected(FIntPoint const FirstVertexCoords, FIntPoint const SecondVertexCoords) const;
	bool IsVertex(FIntPoint const Coords) const;

private:
	using FVertexIndex = int32;

	struct FVertex
	{
		FVertex();

		FVertexIndex AdjacentVertices[4];
	};

	void UpdateAdjacentVertices(FVertexIndex const VertexIndex, FVertexIndex const IndexToUpdateTo);
	void FindMutualSideAndUpdateVertices(FIntPoint const FirstVertexCoords, FIntPoint const SecondVertexCoords, bool const bConnect);

	static FVertexIndex const NoConnection;

	// TODO: Check if it is slower than just having all info in one TMap.
	TMap<FIntPoint, FVertexIndex> CoordsToVertexIndex;
	TMap<FVertexIndex, FIntPoint> VertexIndexToCoords;
	TArray<FVertex> Vertices;
};