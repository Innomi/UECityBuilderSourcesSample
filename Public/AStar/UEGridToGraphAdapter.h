// Fill out your copyright notice in the Description page of Project Settings.

#pragma once 

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "Grid/UEGridDirection.h"
#include "Grid/UEGridSystem.h"

/**
 * FUEGridToGraphAdapter
 */
class UNDEADEMPIRE_API FUEGridToGraphAdapter
{
public:
	using FLocation = FIntPoint;
	using FNodeRef = FLocation;
	using FAStarSearchNode = FGraphAStarDefaultNode<FUEGridToGraphAdapter>;

	FORCEINLINE FUEGridToGraphAdapter(TObjectPtr<UUEGridSystem const> const InGridSystem, EUEGridLayer const InPathGridLayer);

	FORCEINLINE bool IsValidRef(FNodeRef const NodeRef) const;
	FORCEINLINE FNodeRef GetNeighbour(FAStarSearchNode const & SearchNode, int32 const NeighbourIndex) const;
	FORCEINLINE constexpr int32 GetNeighbourCount(FNodeRef const NodeRef) const;

private:
	FORCEINLINE bool IsInRange(FLocation const Location) const;
	FORCEINLINE bool IsBlocked(FLocation const Location) const;

	/*
	 * Note: this is not a USTRUCT nor UCLASS, so pointer won't be nullified at system destruction.
	 * User must ensure that it won't be destroyed.
	 */
	TObjectPtr<UUEGridSystem const> GridSystem;
	EUEGridLayer const PathGridLayer;
};

template <>
FORCEINLINE FGraphAStarDefaultNode<FUEGridToGraphAdapter>::FGraphAStarDefaultNode(FGraphNodeRef const & InNodeRef)
	: NodeRef(InNodeRef)
	, ParentRef(TNumericLimits<FIntPoint::IntType>::Min()) // Some value that won't be ever used as ref.
	, TraversalCost(TNumericLimits<FVector::FReal>::Max())
	, TotalCost(TNumericLimits<FVector::FReal>::Max())
	, SearchNodeIndex(INDEX_NONE)
	, ParentNodeIndex(INDEX_NONE)
	, bIsOpened(false)
	, bIsClosed(false)
{
}

FUEGridToGraphAdapter::FUEGridToGraphAdapter(TObjectPtr<UUEGridSystem const> const InGridSystem, EUEGridLayer const InPathGridLayer)
	: GridSystem(InGridSystem)
	, PathGridLayer(InPathGridLayer)
{
}

bool FUEGridToGraphAdapter::IsValidRef(FNodeRef const NodeRef) const
{
	if (UNLIKELY(!GridSystem))
	{
		return false;
	}
	return IsInRange(NodeRef) && !IsBlocked(NodeRef);
}

FUEGridToGraphAdapter::FNodeRef FUEGridToGraphAdapter::GetNeighbour(FAStarSearchNode const & SearchNode, int32 const NeighbourIndex) const
{
	check(NeighbourIndex < FUEGridDirectionUtil::DirectionsNum);
	return FUEGridDirectionUtil::GetAdjacentCoordsUnsafe(SearchNode.NodeRef, static_cast<EUEGridDirection>(NeighbourIndex));
}

constexpr int32 FUEGridToGraphAdapter::GetNeighbourCount(FNodeRef const NodeRef) const
{
	return FUEGridDirectionUtil::DirectionsNum;
}

bool FUEGridToGraphAdapter::IsInRange(FLocation const Location) const
{
	check(GridSystem);
	return GridSystem->IsInGrid(Location);
}

bool FUEGridToGraphAdapter::IsBlocked(FLocation const Location) const
{
	check(GridSystem);
	bool const bIsNatureObstacle = GridSystem->IsCellOccupied(EUEGridLayer::NatureObstacle, Location);
	bool const bIsConstruction = GridSystem->IsCellOccupied(EUEGridLayer::Construction, Location);
	bool const bIsPath = GridSystem->IsCellOccupied(PathGridLayer, Location);
	return bIsNatureObstacle || (bIsConstruction && !bIsPath);
}