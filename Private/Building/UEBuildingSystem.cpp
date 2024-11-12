// Fill out your copyright notice in the Description page of Project Settings.

#include "Building/UEBuildingSystem.h"
#include "Grid/UEConcurrentSpatialGridIndex.h"

UUEBuildingSystem::UUEBuildingSystem() = default;

UUEBuildingSystem::UUEBuildingSystem(FVTableHelper & Helper)
{
}

UUEBuildingSystem::~UUEBuildingSystem() = default;

void UUEBuildingSystem::Initialize(FSubsystemCollectionBase & Collection)
{
	Super::Initialize(Collection);

	// TODO : Create config.
	FIntPoint const IndexSize = { 1024, 1024 };
	FIntPoint const IndexCellSize = { 16, 16 };
	FIntPoint const LockCellSize = { 64, 64 };
	SpatialGridIndex.Reset(new TUEConcurrentSpatialGridIndex<TObjectPtr<AActor>>(IndexSize, IndexCellSize, LockCellSize));
	TaskPipe.Reset(new UE::Tasks::FPipe(UE_SOURCE_LOCATION));
}

void UUEBuildingSystem::Deinitialize()
{
	if (TaskPipe)
	{
		TaskPipe->WaitUntilEmpty();
	}
	TaskPipe.Reset();
	SpatialGridIndex.Reset();

	Super::Deinitialize();
}

FIntPoint UUEBuildingSystem::GetIndexOriginCoords() const
{
	FIntPoint const GridIndexSize = GetIndexSize();
	return GridIndexSize != FIntPoint{ 0, 0 } ? GridIndexSize / -2 : FIntPoint{ 0, 0 };
}

FIntPoint UUEBuildingSystem::GetIndexSize() const
{
	return SpatialGridIndex ? SpatialGridIndex->GetSize() : FIntPoint{ 0, 0 };
}

void UUEBuildingSystem::AddBuildingAsync(TObjectPtr<AActor> const BuildingPtr, FIntRect const & BuildingRect)
{
	if (TaskPipe)
	{
		TaskPipe->Launch(UE_SOURCE_LOCATION,
			[this, BuildingPtr, BuildingRect]()
			{
				AddBuilding(BuildingPtr, BuildingRect);
			});
	}
}

void UUEBuildingSystem::RemoveOverlappedBuildingsAsync(FIntRect const & RectToCheckForOverlap)
{
	if (TaskPipe)
	{
		TaskPipe->Launch(UE_SOURCE_LOCATION,
			[this, RectToCheckForOverlap]()
			{
				RemoveOverlappedBuildings(RectToCheckForOverlap);
			});
	}
}

void UUEBuildingSystem::RemoveOverlappedBuildingsAsync(FIntRect const & RectToCheckForOverlap, TObjectPtr<AActor> const BuildingPtr)
{
	if (TaskPipe)
	{
		TaskPipe->Launch(UE_SOURCE_LOCATION,
			[this, RectToCheckForOverlap, BuildingPtr]()
			{
				RemoveOverlappedBuildings(RectToCheckForOverlap, BuildingPtr);
			});
	}
}

bool UUEBuildingSystem::AddBuilding(TObjectPtr<AActor> const BuildingPtr, FIntRect const & BuildingRect)
{
	if (SpatialGridIndex)
	{
		return SpatialGridIndex->TryInsert(BuildingRect - GetIndexOriginCoords(), BuildingPtr);
	}
	return false;
}

void UUEBuildingSystem::RemoveOverlappedBuildings(FIntRect const & RectToCheckForOverlap)
{
	if (SpatialGridIndex)
	{
		SpatialGridIndex->Erase(RectToCheckForOverlap - GetIndexOriginCoords());
	}
}

void UUEBuildingSystem::RemoveOverlappedBuildings(FIntRect const & RectToCheckForOverlap, TObjectPtr<AActor> const BuildingPtr)
{
	if (SpatialGridIndex)
	{
		SpatialGridIndex->EraseByPredicate(RectToCheckForOverlap - GetIndexOriginCoords(), [BuildingPtr](TPair<FIntRect, TObjectPtr<AActor>> const & IndexEntry) -> bool
			{
				return IndexEntry.Value == BuildingPtr;
			});
	}
}

TArray<TObjectPtr<AActor>> UUEBuildingSystem::GetOverlappedBuildings(FIntRect const & RectToCheckForOverlap) const
{
	TArray<TObjectPtr<AActor>> Buildings;
	if (SpatialGridIndex)
	{
		SpatialGridIndex->GetOverlapping(RectToCheckForOverlap - GetIndexOriginCoords()).GenerateValueArray(Buildings);
	}
	return Buildings;
}
