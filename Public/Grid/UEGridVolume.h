// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Common/UEActor.h"
#include "UEGridVolume.generated.h"

class UBoxComponent;
class UUEGridComponent;

/**
 * Volume to provide axis aligned grid for specified space. Should have zero rotation.
 */
UCLASS(Blueprintable, HideCategories = (Actor, Collision, Cooking, HLOD, Input, LOD, Networking, Physics, Replication))
class UNDEADEMPIRE_API AUEGridVolume : public AUEActor
{
	GENERATED_BODY()
	
public:
	AUEGridVolume();

	virtual void PreRegisterAllComponents() override;

protected:
	virtual bool NeedsLoadForServer() const override;
	virtual bool IsLevelBoundsRelevant() const override;

	/** Component that owns the grid. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UE Grid")
	TObjectPtr<UUEGridComponent> Grid;

#if WITH_EDITORONLY_DATA
	/** Box for visualizing grid area extents. */
	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> Bounds;
#endif
};
