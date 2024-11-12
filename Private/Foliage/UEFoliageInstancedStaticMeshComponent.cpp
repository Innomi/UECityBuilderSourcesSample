// Fill out your copyright notice in the Description page of Project Settings.

#include "Foliage/UEFoliageInstancedStaticMeshComponent.h"

UUEFoliageInstancedStaticMeshComponent::UUEFoliageInstancedStaticMeshComponent()
{
	SetCollisionProfileName("Foliage");
	SetNumCustomDataFloats(1);
}

bool UUEFoliageInstancedStaticMeshComponent::SetInstanceVisibility(int32 const InstanceIndex, bool const bIsVisible, bool const bMarkRenderStateDirty /* = false */)
{
	return SetCustomDataValue(InstanceIndex, HiddenFlagCustomDataIndex, bIsVisible ? 0.f : 1.f, bMarkRenderStateDirty);
}

bool UUEFoliageInstancedStaticMeshComponent::SetInstancesVisibility(TArray<int32> const InstanceIndices, bool const bIsVisible, bool const bMarkRenderStateDirty /* = false */)
{
	bool bChanged = false;
	for (int32 const InstanceIndex : InstanceIndices)
	{
		bChanged |= SetInstanceVisibility(InstanceIndex, bIsVisible);
	}
	if (bChanged && bMarkRenderStateDirty)
	{
		MarkRenderStateDirty();
	}
	return bChanged;
}