// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Path/UEPathGraph.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tasks/Pipe.h"
#include "UEPathSystem.generated.h"

class AUEPathActor;

UENUM()
enum class EUEPathGraph : uint8
{
	Road,
	GRAPHS_NUM UMETA(Hidden)
};

/**
 * UUEPathSystem
 */
UCLASS()
class UNDEADEMPIRE_API UUEPathSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase & Collection) override;
	virtual void Deinitialize() override;
	
	void RegisterActor(TObjectPtr<AUEPathActor> const PathActor, EUEPathGraph const PathGraph);
	void UnregisterActor(TObjectPtr<AUEPathActor> const PathActor, EUEPathGraph const PathGraph);

	TObjectPtr<AUEPathActor> GetPathActor(TSubclassOf<AUEPathActor> const PathActorClass) const;
	TArray<TObjectPtr<AUEPathActor>> const & GetPathActors(EUEPathGraph const PathGraph) const;
	void UpdateGraphAsync(EUEPathGraph const PathGraph, TArray<FIntPoint> && VerticesToAdd, TArray<FIntPoint> && VerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> && ConnectionsToAdd, TArray<TTuple<FIntPoint, FIntPoint>> && ConnectionsToRemove);

private:
	FUEPathGraph & GetGraph(EUEPathGraph const PathGraph);
	FUEPathGraph const & GetGraph(EUEPathGraph const PathGraph) const;
	FRWLock & GetGraphLock(EUEPathGraph const PathGraph) const;
	void UpdateGraph(EUEPathGraph const PathGraph, TArray<FIntPoint> const & VerticesToAdd, TArray<FIntPoint> const & VerticesToRemove, TArray<TTuple<FIntPoint, FIntPoint>> const & ConnectionsToAdd, TArray<TTuple<FIntPoint, FIntPoint>> const & ConnectionsToRemove);

	TArray<TObjectPtr<AUEPathActor>> PathActors[static_cast<uint8>(EUEPathGraph::GRAPHS_NUM)];
	TArray<UE::Tasks::FPipe> TaskPipes;
	TArray<FUEPathGraph> PathGraphs;
	mutable TArray<FRWLock> GraphLocks;
};