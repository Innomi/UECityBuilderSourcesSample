// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UEResourceStorageComponent.generated.h"

class UUEResourceDataAsset;

/**
 * Component that allows actor to store resources. Contains information about what resources can be stored,
 * quantity of each stored resource type and maximum possible summary quantity of all stored resources.
 */
UCLASS()
class UNDEADEMPIRE_API UUEResourceStorageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUEResourceStorageComponent();

	virtual void BeginPlay() override;

	bool AddResource(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity);
	uint64 AddResourceWithRemainder(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity);
	bool AddResources(TMap<UUEResourceDataAsset const *, uint64> const & Resources);
	bool SubtractResource(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity);
	uint64 SubtractResourceWithRemainder(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity);
	bool SubtractResources(TMap<UUEResourceDataAsset const *, uint64> const & Resources);
	bool Contains(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity) const;
	bool Contains(TMap<UUEResourceDataAsset const *, uint64> const & Resources) const;
	uint64 GetResourceQuantity(UUEResourceDataAsset const * const ResourceDataAsset) const;
	bool EmptyResource(UUEResourceDataAsset const * const ResourceDataAsset);
	void EmptyAllResources();
	uint64 GetMaxResourcesQuantity() const;
	uint64 GetResourcesQuantity() const;
	bool SetMaxResourcesQuantity(uint64 const NewMaxResourcesQuantity = 0);
	bool CanReceive(uint64 const ResourceQuantity) const;
	bool CanReceive(UUEResourceDataAsset const * ResourceDataAsset, uint64 const ResourceQuantity) const;
	bool CanReceive(TMap<UUEResourceDataAsset const *, uint64> const & Resources) const;
	bool IsEmpty() const;
	bool CanStore(UUEResourceDataAsset const * const ResourceDataAsset) const;
	bool AddNewOrSetQuantityOfResourceType(UUEResourceDataAsset const * const ResourceDataAsset, uint64 const ResourceQuantity = 0);
	bool RemoveResourceType(UUEResourceDataAsset const * const ResourceDataAsset);
	void RemoveAllResourceTypes();
	TArray<UUEResourceDataAsset const *> const & GetResourceTypes() const;
	void TransferResourcesTo(UUEResourceStorageComponent & ResourceStorage);

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent) override;
#endif

protected:
	void AddResourceChecked(int32 const ResourceIndex, uint64 const QuantityToAdd);
	void SubtractResourceChecked(int32 const ResourceIndex, uint64 const QuantityToSubtract);

	// Data Assets of resources that can be stored in this storage.
	UPROPERTY(EditDefaultsOnly, Category = "UE|Resource Storage", meta = (EditFixedOrder, NoElementDuplicate))
	TArray<TObjectPtr<UUEResourceDataAsset const>> ResourceDataAssets;

	// Quantities of resources stored in this storage.
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "UE|Resource Storage", meta = (EditFixedOrder))
	TArray<uint64> ResourceQuantities;

	// Max quantity of resources that can be stored in this storage.
	UPROPERTY(EditDefaultsOnly, Category = "UE|Resource Storage")
	uint64 MaxResourcesQuantity;

	// Quantity of resources currenntly stored in this storage.
	UPROPERTY(VisibleInstanceOnly, Category = "UE|Resource Storage")
	uint64 ResourcesQuantity;
};
