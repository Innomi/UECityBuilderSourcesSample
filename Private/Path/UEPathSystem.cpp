// Fill out your copyright notice in the Description page of Project Settings.

#include "Path/UEPathSystem.h"
#include "Common/UELog.h"
#include "GameModes/UEGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Path/UEPathActor.h"
#include "Path/UEPathDataAsset.h"
#include "Path/UEPathGraph.h"
#include "Tasks/Pipe.h"
#include "Tasks/Task.h"

void UUEPathSystem::Initialize(FSubsystemCollectionBase & Collection)
{
	Super::Initialize(Collection);

	for (uint8 PipeIndex = 0; PipeIndex < static_cast<uint8>(EUEPathGraph::GRAPHS_NUM); ++PipeIndex)
	{
		TaskPipes.Emplace(UE_SOURCE_LOCATION);
	}
	PathGraphs.AddDefaulted(static_cast<uint8>(EUEPathGraph::GRAPHS_NUM));
	GraphLocks.AddDefaulted(static_cast<uint8>(EUEPathGraph::GRAPHS_NUM));
}

void UUEPathSystem::Deinitialize()
{
	for (UE::Tasks::FPipe & Pipe : TaskPipes)
	{
		Pipe.WaitUntilEmpty();
	}
	GraphLocks.Empty();
	PathGraphs.Empty();
	TaskPipes.Empty();
	for (TArray<TObjectPtr<AUEPathActor>> & PathActorsOfExactLayer : PathActors)
	{
		PathActorsOfExactLayer.Empty();
	}

	Super::Deinitialize();
}

void UUEPathSystem::RegisterActor(TObjectPtr<AUEPathActor> const PathActor, EUEPathGraph const PathGraph)
{
	uint8 const UintPathGraph = static_cast<uint8>(PathGraph);
	check(UintPathGraph < static_cast<uint8>(EUEPathGraph::GRAPHS_NUM));
	if (IsValid(PathActor))
	{
		PathActors[UintPathGraph].AddUnique(PathActor);
	}
}

void UUEPathSystem::UnregisterActor(TObjectPtr<AUEPathActor> const PathActor, EUEPathGraph const PathGraph)
{
	uint8 const UintPathGraph = static_cast<uint8>(PathGraph);
	check(UintPathGraph < static_cast<uint8>(EUEPathGraph::GRAPHS_NUM));
	PathActors[UintPathGraph].Remove(PathActor);
}

TObjectPtr<AUEPathActor> UUEPathSystem::GetPathActor(TSubclassOf<AUEPathActor> const PathActorClass) const
{
	for (TArray<TObjectPtr<AUEPathActor>> const & PathActorsOfExactLayer : PathActors)
	{
		for (TObjectPtr<AUEPathActor> const PathActor : PathActorsOfExactLayer)
		{
			if (IsValid(PathActor) && PathActor->GetClass() == PathActorClass)
			{
				return PathActor;
			}
		}
	}
	return nullptr;
}

TArray<TObjectPtr<AUEPathActor>> const & UUEPathSystem::GetPathActors(EUEPathGraph const PathGraph) const
{
	uint8 const UintPathGraph = static_cast<uint8>(PathGraph);
	check(UintPathGraph < static_cast<uint8>(EUEPathGraph::GRAPHS_NUM));
	return PathActors[UintPathGraph];
}

void UUEPathSystem::UpdateGraphAsync(EUEPathGraph const PathGraph, TArray<FIntPoint> && VerticesToAdd, TArray<FIntPoint> && VerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> && ConnectionsToAdd, TArray<TTuple<FIntPoint, FIntPoint>> && ConnectionsToRemove)
{
	TaskPipes[static_cast<uint8>(PathGraph)].Launch(UE_SOURCE_LOCATION,
		[this, PathGraph, VerticesToAdd = MoveTemp(VerticesToAdd), VerticesToRemove = MoveTemp(VerticesToRemove), ConnectionsToAdd = MoveTemp(ConnectionsToAdd), ConnectionsToRemove = MoveTemp(ConnectionsToRemove)] ()
		{
			UpdateGraph(PathGraph, VerticesToAdd, VerticesToRemove, ConnectionsToAdd, ConnectionsToRemove);
		});
}

FUEPathGraph & UUEPathSystem::GetGraph(EUEPathGraph const PathGraph)
{
	return PathGraphs[static_cast<uint8>(PathGraph)];
}

FUEPathGraph const & UUEPathSystem::GetGraph(EUEPathGraph const PathGraph) const
{
	return PathGraphs[static_cast<uint8>(PathGraph)];
}

FRWLock & UUEPathSystem::GetGraphLock(EUEPathGraph const PathGraph) const
{
	return GraphLocks[static_cast<uint8>(PathGraph)];
}

void UUEPathSystem::UpdateGraph(EUEPathGraph const PathGraph, TArray<FIntPoint> const & VerticesToAdd, TArray<FIntPoint> const & VerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> const & ConnectionsToAdd, TArray<TTuple<FIntPoint, FIntPoint>> const & ConnectionsToRemove)
{
	FRWScopeLock WScopeLock(GetGraphLock(PathGraph), FRWScopeLockType::SLT_Write);
	FUEPathGraph & Graph = GetGraph(PathGraph);
	for (TTuple<FIntPoint, FIntPoint> const & Connection : ConnectionsToRemove)
	{
		Graph.DisconnectVertices(Connection.Get<0>(), Connection.Get<1>());
	}
	for (FIntPoint const VertexCoords : VerticesToRemove)
	{
		Graph.RemoveVertex(VertexCoords);
	}
	for (FIntPoint const VertexCoords : VerticesToAdd)
	{
		Graph.AddVertex(VertexCoords);
	}
	for (TTuple<FIntPoint, FIntPoint> const & Connection : ConnectionsToAdd)
	{
		Graph.ConnectVertices(Connection.Get<0>(), Connection.Get<1>());
	}
}
