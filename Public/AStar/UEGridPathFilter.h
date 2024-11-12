// Fill out your copyright notice in the Description page of Project Settings.

#pragma once 

#include "AStar/UEGridToGraphAdapter.h"
#include "CoreMinimal.h"
#include "GraphAStar.h"

/**
 * FUEGridPathFilter
 */
class UNDEADEMPIRE_API FUEGridPathFilter
{
public:
	using FGraph = FUEGridToGraphAdapter;
	using FNodeRef = FGraph::FNodeRef;
	using FAStarSearchNode = FGraphAStarDefaultNode<FGraph>;

	FORCEINLINE explicit FUEGridPathFilter(FVector::FReal const InHeuristicScale = 1., FVector2D const InAxiswiseHeuristicScale = { 1., 1. }, uint32 const InMaxSearchNodes = 512);
	FORCEINLINE FVector::FReal GetHeuristicScale() const;
	FORCEINLINE FVector::FReal GetHeuristicCost(FAStarSearchNode const & StartNode, FAStarSearchNode const & EndNode) const;
	FORCEINLINE FVector::FReal GetTraversalCost(FAStarSearchNode const & StartNode, FAStarSearchNode const & EndNode) const;
	FORCEINLINE bool IsTraversalAllowed(FNodeRef const NodeA, FNodeRef const NodeB) const;
	FORCEINLINE bool WantsPartialSolution() const;
	FORCEINLINE bool ShouldIgnoreClosedNodes() const;
	FORCEINLINE bool ShouldIncludeStartNodeInPath() const;
	FORCEINLINE uint32 GetMaxSearchNodes() const;

private:
	FVector::FReal const HeuristicScale;
	FVector2D const AxiswiseHeuristicScale;
	uint32 const MaxSearchNodes;
};

FUEGridPathFilter::FUEGridPathFilter(FVector::FReal const InHeuristicScale, FVector2D const InAxiswiseHeuristicScale, uint32 const InMaxSearchNodes)
	: HeuristicScale(InHeuristicScale)
	, AxiswiseHeuristicScale(InAxiswiseHeuristicScale)
	, MaxSearchNodes(InMaxSearchNodes)
{
}

FVector::FReal FUEGridPathFilter::GetHeuristicScale() const
{
	return HeuristicScale;
}

FVector::FReal FUEGridPathFilter::GetHeuristicCost(FAStarSearchNode const & StartNode, FAStarSearchNode const & EndNode) const
{
	FNodeRef const StartNodeRef = StartNode.NodeRef;
	FNodeRef const EndNodeRef = EndNode.NodeRef;
	FVector::FReal const XAxisPart = static_cast<FVector::FReal>(FMath::Abs(StartNodeRef.X - EndNodeRef.X)) * AxiswiseHeuristicScale.X;
	FVector::FReal const YAxisPart = static_cast<FVector::FReal>(FMath::Abs(StartNodeRef.Y - EndNodeRef.Y)) * AxiswiseHeuristicScale.Y;
	return XAxisPart + YAxisPart;
}

FVector::FReal FUEGridPathFilter::GetTraversalCost(FAStarSearchNode const & StartNode, FAStarSearchNode const & EndNode) const
{
	check((EndNode.NodeRef - StartNode.NodeRef).SizeSquared() == 1);
	return 1.;
}

bool FUEGridPathFilter::IsTraversalAllowed(FNodeRef const NodeA, FNodeRef const NodeB) const
{
	check((NodeB - NodeA).SizeSquared() == 1);
	return true;
}

bool FUEGridPathFilter::WantsPartialSolution() const
{
	return false;
}

bool FUEGridPathFilter::ShouldIgnoreClosedNodes() const
{
	return true;
}

bool FUEGridPathFilter::ShouldIncludeStartNodeInPath() const
{
	return true;
}

uint32 FUEGridPathFilter::GetMaxSearchNodes() const
{
	return MaxSearchNodes;
}