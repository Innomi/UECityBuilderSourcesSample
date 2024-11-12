// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UEBuildingComponent.generated.h"

class UUEBuildingSystem;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNDEADEMPIRE_API UUEBuildingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	TObjectPtr<UUEBuildingSystem> GetBuildingSystem() const;
};
