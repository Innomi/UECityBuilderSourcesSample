// Fill out your copyright notice in the Description page of Project Settings.

#include "Economy/UEResourceStorageComponent.h"
#include "Common/UELog.h"

namespace
{
	void ValidateResourceStorage(UUEResourceStorageComponent const & Component)
	{
#if WITH_EDITOR
		TArray<UUEResourceDataAsset const *> UniqueDataAssets;
		for (UUEResourceDataAsset const * ResourceDataAsset : Component.GetResourceTypes())
		{
			UniqueDataAssets.AddUnique(ResourceDataAsset);
			if (!ResourceDataAsset)
			{
				UE_LOGFMT(LogUE, Warning, "Null UEResourceDataAsset in \"{0}\".", Component.GetName());
			}
		}
		if (UniqueDataAssets.Num() != Component.GetResourceTypes().Num())
		{
			UE_LOGFMT(LogUE, Warning, "Duplicated UEResourceDataAssets in \"{0}\".", Component.GetName());
		}
#endif
	}
} // namespace

UUEResourceStorageComponent::UUEResourceStorageComponent()
	: MaxResourcesQuantity{}
	, ResourcesQuantity{}
{
}

void UUEResourceStorageComponent::BeginPlay()
{
	Super::BeginPlay();

	ValidateResourceStorage(*this);
}

bool UUEResourceStorageComponent::AddResource(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity)
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	if (!CanReceive(ResourceQuantity))
	{
		return false;
	}
	if (int32 const ResourceIndex = ResourceDataAssets.IndexOfByKey(ResourceDataAsset); ResourceIndex != INDEX_NONE)
	{
		AddResourceChecked(ResourceIndex, ResourceQuantity);
		return true;
	}
	return false;
}

uint64 UUEResourceStorageComponent::AddResourceWithRemainder(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity)
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	uint64 const QuantityToAdd = FMath::Min(ResourceQuantity, MaxResourcesQuantity - ResourcesQuantity);
	if (QuantityToAdd == 0)
	{
		return ResourceQuantity;
	}
	if (int32 const ResourceIndex = ResourceDataAssets.IndexOfByKey(ResourceDataAsset); ResourceIndex != INDEX_NONE)
	{
		AddResourceChecked(ResourceIndex, QuantityToAdd);
		return ResourceQuantity - QuantityToAdd;
	}
	return ResourceQuantity;
}

bool UUEResourceStorageComponent::AddResources(TMap<UUEResourceDataAsset const *, uint64> const & Resources)
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	if (CanReceive(Resources))
	{
		for (TPair<UUEResourceDataAsset const *, uint64> const & ResourcePair : Resources)
		{
			AddResourceChecked(ResourceDataAssets.IndexOfByKey(ResourcePair.Key), ResourcePair.Value);
		}
		return true;
	}
	return false;
}

bool UUEResourceStorageComponent::SubtractResource(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity)
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	if (int32 const ResourceIndex = ResourceDataAssets.IndexOfByKey(ResourceDataAsset); ResourceIndex != INDEX_NONE)
	{
		if (ResourceQuantities[ResourceIndex] < ResourceQuantity)
		{
			return false;
		}
		SubtractResourceChecked(ResourceIndex, ResourceQuantity);
		return true;
	}
	return false;
}

uint64 UUEResourceStorageComponent::SubtractResourceWithRemainder(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity)
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	if (int32 const ResourceIndex = ResourceDataAssets.IndexOfByKey(ResourceDataAsset); ResourceIndex != INDEX_NONE)
	{
		uint64 const QuantityToSubtract = FMath::Min(ResourceQuantity, ResourceQuantities[ResourceIndex]);
		SubtractResourceChecked(ResourceIndex, QuantityToSubtract);
		return ResourceQuantity - QuantityToSubtract;
	}
	return ResourceQuantity;
}

bool UUEResourceStorageComponent::SubtractResources(TMap<UUEResourceDataAsset const *, uint64> const & Resources)
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	if (Contains(Resources))
	{
		for (TPair<UUEResourceDataAsset const *, uint64> const & ResourcePair : Resources)
		{
			SubtractResourceChecked(ResourceDataAssets.IndexOfByKey(ResourcePair.Key), ResourcePair.Value);
		}
		return true;
	}
	return false;
}

bool UUEResourceStorageComponent::Contains(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity) const
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	int32 const ResourceIndex = ResourceDataAssets.IndexOfByKey(ResourceDataAsset);
	return ResourceIndex != INDEX_NONE && ResourceQuantities[ResourceIndex] >= ResourceQuantity;
}

bool UUEResourceStorageComponent::Contains(TMap<UUEResourceDataAsset const *, uint64> const & Resources) const
{
	for (TPair<UUEResourceDataAsset const *, uint64> const & ResourcePair : Resources)
	{
		if (!Contains(ResourcePair.Key, ResourcePair.Value))
		{
			return false;
		}
	}
	return true;
}

uint64 UUEResourceStorageComponent::GetResourceQuantity(UUEResourceDataAsset const * const ResourceDataAsset) const
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	if (int32 const ResourceIndex = ResourceDataAssets.IndexOfByKey(ResourceDataAsset); ResourceIndex != INDEX_NONE)
	{
		return ResourceQuantities[ResourceIndex];
	}
	return uint64{};
}

bool UUEResourceStorageComponent::EmptyResource(UUEResourceDataAsset const * const ResourceDataAsset)
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	if (int32 const ResourceIndex = ResourceDataAssets.IndexOfByKey(ResourceDataAsset); ResourceIndex != INDEX_NONE)
	{
		check(ResourcesQuantity >= ResourceQuantities[ResourceIndex]);
		ResourcesQuantity -= ResourceQuantities[ResourceIndex];
		ResourceQuantities[ResourceIndex] = 0;
		return true;
	}
	return false;
}

void UUEResourceStorageComponent::EmptyAllResources()
{
	for (uint64 & ResourceQuantity : ResourceQuantities)
	{
		ResourceQuantity = 0;
	}
	ResourcesQuantity = 0;
}

uint64 UUEResourceStorageComponent::GetMaxResourcesQuantity() const
{
	return MaxResourcesQuantity;
}

uint64 UUEResourceStorageComponent::GetResourcesQuantity() const
{
	return ResourcesQuantity;
}

bool UUEResourceStorageComponent::SetMaxResourcesQuantity(uint64 const NewMaxResourcesQuantity)
{
	if (NewMaxResourcesQuantity >= ResourcesQuantity)
	{
		MaxResourcesQuantity = NewMaxResourcesQuantity;
		return true;
	}
	return false;
}

bool UUEResourceStorageComponent::CanReceive(uint64 const ResourceQuantity) const
{
	return ResourceQuantity <= MaxResourcesQuantity - ResourcesQuantity;
}

bool UUEResourceStorageComponent::CanReceive(UUEResourceDataAsset const * ResourceDataAsset, uint64 const ResourceQuantity) const
{
	return CanReceive(ResourceQuantity) && CanStore(ResourceDataAsset);
}

bool UUEResourceStorageComponent::CanReceive(TMap<UUEResourceDataAsset const *, uint64> const & Resources) const
{
	check(MaxResourcesQuantity >= ResourcesQuantity);
	uint64 AvailableToReceiveResourcesQuantity = MaxResourcesQuantity - ResourcesQuantity;
	for (TPair<UUEResourceDataAsset const *, uint64> const & ResourcePair : Resources)
	{
		if (!CanStore(ResourcePair.Key) || AvailableToReceiveResourcesQuantity < ResourcePair.Value)
		{
			return false;
		}
		AvailableToReceiveResourcesQuantity -= ResourcePair.Value;
	}
	return true;
}

bool UUEResourceStorageComponent::IsEmpty() const
{
	return ResourcesQuantity == 0;
}

bool UUEResourceStorageComponent::CanStore(UUEResourceDataAsset const * const ResourceDataAsset) const
{
	return ResourceDataAssets.Contains(ResourceDataAsset);
}

bool UUEResourceStorageComponent::AddNewOrSetQuantityOfResourceType(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity)
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	if (int32 const ResourceIndex = ResourceDataAssets.IndexOfByKey(ResourceDataAsset); ResourceIndex != INDEX_NONE)
	{
		check(ResourcesQuantity >= ResourceQuantities[ResourceIndex]);
		uint64 const ResourcesQuantityWithoutChangedResource = ResourcesQuantity - ResourceQuantities[ResourceIndex];
		if (ResourceQuantity > MaxResourcesQuantity - ResourcesQuantityWithoutChangedResource)
		{
			return false;
		}
		ResourceQuantities[ResourceIndex] = ResourceQuantity;
		ResourcesQuantity = ResourcesQuantityWithoutChangedResource + ResourceQuantity;
		return true;
	}
	if (CanReceive(ResourceQuantity))
	{
		ResourceDataAssets.Emplace(ResourceDataAsset);
		ResourceQuantities.Emplace(ResourceQuantity);
		ResourcesQuantity += ResourceQuantity;
		return true;
	}
	return false;
}

bool UUEResourceStorageComponent::RemoveResourceType(UUEResourceDataAsset const * const ResourceDataAsset)
{
	check(ResourceDataAssets.Num() == ResourceQuantities.Num());
	if (int32 const ResourceIndex = ResourceDataAssets.IndexOfByKey(ResourceDataAsset); ResourceIndex != INDEX_NONE)
	{
		ResourceDataAssets.RemoveAt(ResourceIndex);
		check(ResourcesQuantity >= ResourceQuantities[ResourceIndex]);
		ResourcesQuantity -= ResourceQuantities[ResourceIndex];
		ResourceQuantities.RemoveAt(ResourceIndex);
		return true;
	}
	return false;
}

void UUEResourceStorageComponent::RemoveAllResourceTypes()
{
	ResourceDataAssets.Empty();
	ResourceQuantities.Empty();
	ResourcesQuantity = 0;
}

TArray<UUEResourceDataAsset const *> const & UUEResourceStorageComponent::GetResourceTypes() const
{
	return ResourceDataAssets;
}

void UUEResourceStorageComponent::TransferResourcesTo(UUEResourceStorageComponent & ResourceStorage)
{
	for (int32 ResourceIndex = 0; ResourceIndex < ResourceDataAssets.Num(); ++ResourceIndex)
	{
		uint64 const Remainder = ResourceStorage.AddResourceWithRemainder(ResourceDataAssets[ResourceIndex], ResourceQuantities[ResourceIndex]);
		SubtractResourceChecked(ResourceIndex, ResourceQuantities[ResourceIndex] - Remainder);
	}
}

#if WITH_EDITOR
void UUEResourceStorageComponent::PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (FName const PropertyName = PropertyChangedEvent.GetPropertyName();
		PropertyName == GET_MEMBER_NAME_CHECKED(UUEResourceStorageComponent, ResourceDataAssets))
	{
		switch (PropertyChangedEvent.ChangeType)
		{
			case EPropertyChangeType::ArrayAdd:
			{
				ResourceQuantities.Emplace(0);
				break;
			}
			case EPropertyChangeType::ArrayRemove:
			{
				int32 const RemovedDataAssetIndex = PropertyChangedEvent.GetArrayIndex(PropertyName.ToString());
				check(ResourceQuantities[RemovedDataAssetIndex] <= ResourcesQuantity);
				ResourcesQuantity -= ResourceQuantities[RemovedDataAssetIndex];
				ResourceQuantities.RemoveAt(RemovedDataAssetIndex);
				break;
			}
			case EPropertyChangeType::ArrayClear:
			{
				ResourceQuantities.Empty();
				ResourcesQuantity = 0;
				break;
			}
			default:
			{
			}
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UUEResourceStorageComponent, ResourceQuantities))
	{
		switch (PropertyChangedEvent.ChangeType)
		{
			case EPropertyChangeType::ValueSet:
			{
				ResourcesQuantity = 0;
				for (uint64 const ResourceQuantity : ResourceQuantities)
				{
					ResourcesQuantity += ResourceQuantity;
				}
				int32 const ModifiedResourceQuantityIndex = PropertyChangedEvent.GetArrayIndex(PropertyName.ToString());
				// Also checks if overflow happen.
				if (ResourcesQuantity > MaxResourcesQuantity || ResourcesQuantity < ResourceQuantities[ModifiedResourceQuantityIndex])
				{
					ResourceQuantities[ModifiedResourceQuantityIndex] = MaxResourcesQuantity - (ResourcesQuantity - ResourceQuantities[ModifiedResourceQuantityIndex]);
					ResourcesQuantity = MaxResourcesQuantity;
				}
				break;
			}
			default:
			{
			}
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UUEResourceStorageComponent, MaxResourcesQuantity))
	{
		if (MaxResourcesQuantity < ResourcesQuantity)
		{
			MaxResourcesQuantity = ResourcesQuantity;
		}
	}
}
#endif

void UUEResourceStorageComponent::AddResourceChecked(int32 const ResourceIndex, uint64 const QuantityToAdd)
{
	ResourceQuantities[ResourceIndex] += QuantityToAdd;
	ResourcesQuantity += QuantityToAdd;
}

void UUEResourceStorageComponent::SubtractResourceChecked(int32 const ResourceIndex, uint64 const QuantityToSubtract)
{
	ResourceQuantities[ResourceIndex] -= QuantityToSubtract;
	ResourcesQuantity -= QuantityToSubtract;
}
