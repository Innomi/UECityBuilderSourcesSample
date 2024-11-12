// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <concepts>

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UEBuildingSystem.generated.h"

template <typename>
class TUEConcurrentSpatialGridIndex;

/**
 * UUEBuildingSystem
 */
UCLASS()
class UNDEADEMPIRE_API UUEBuildingSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UUEBuildingSystem();
	// Workaround for TUniquePtr to work properly, see UniquePtr.h.
	UUEBuildingSystem(FVTableHelper & Helper);
	virtual ~UUEBuildingSystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FIntPoint GetIndexOriginCoords() const;
	FIntPoint GetIndexSize() const;
	void AddBuildingAsync(TObjectPtr<AActor> const BuildingPtr, FIntRect const & BuildingRect);
	void RemoveOverlappedBuildingsAsync(FIntRect const & RectToCheckForOverlap);

	/*
	 * Takes overlapped buildings and removes only corresponding to BuildingPtr.
	 */
	void RemoveOverlappedBuildingsAsync(FIntRect const & RectToCheckForOverlap, TObjectPtr<AActor> const BuildingPtr);

	template <std::invocable<TArray<TObjectPtr<AActor>> &&> CallbackType>
	FORCEINLINE void GetOverlappedBuildingsAsync(FIntRect const & RectToCheckForOverlap, CallbackType && Callback) const;

protected:
	bool AddBuilding(TObjectPtr<AActor> const BuildingPtr, FIntRect const & BuildingRect);
	void RemoveOverlappedBuildings(FIntRect const & RectToCheckForOverlap);
	void RemoveOverlappedBuildings(FIntRect const & RectToCheckForOverlap, TObjectPtr<AActor> const BuildingPtr);
	TArray<TObjectPtr<AActor>> GetOverlappedBuildings(FIntRect const & RectToCheckForOverlap) const;

private:
	TUniquePtr<UE::Tasks::FPipe> TaskPipe;
	TUniquePtr<TUEConcurrentSpatialGridIndex<TObjectPtr<AActor>>> SpatialGridIndex;
};

template <std::invocable<TArray<TObjectPtr<AActor>> &&> CallbackType>
void UUEBuildingSystem::GetOverlappedBuildingsAsync(FIntRect const & RectToCheckForOverlap, CallbackType && Callback) const
{
	if (TaskPipe)
	{
		TaskPipe->Launch(UE_SOURCE_LOCATION,
			[this, RectToCheckForOverlap, Callback = Forward<CallbackType>(Callback)]()
			{
				Callback(GetOverlappedBuildings(RectToCheckForOverlap));
			});
	}
}
