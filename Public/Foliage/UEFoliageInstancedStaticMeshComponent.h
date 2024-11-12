// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FoliageInstancedStaticMeshComponent.h"
#include "UEFoliageInstancedStaticMeshComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNDEADEMPIRE_API UUEFoliageInstancedStaticMeshComponent : public UFoliageInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UUEFoliageInstancedStaticMeshComponent();

	bool SetInstanceVisibility(int32 const InstanceIndex, bool const bIsVisible, bool const bMarkRenderStateDirty = false);
	bool SetInstancesVisibility(TArray<int32> const InstanceIndices, bool const bIsVisible, bool const bMarkRenderStateDirty = false);

	static int32 const HiddenFlagCustomDataIndex = 0;
};