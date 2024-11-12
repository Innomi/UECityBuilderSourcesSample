// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEGridVolume.h"
#include "Common/UELog.h"
#include "Components/BoxComponent.h"
#include "Grid/UEGridComponent.h"

AUEGridVolume::AUEGridVolume()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("RootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);

	Grid = CreateDefaultSubobject<UUEGridComponent>("Grid");
	Grid->SetMobility(EComponentMobility::Static);
	Grid->SetupAttachment(RootComponent);

#if WITH_EDITORONLY_DATA
	// Add box for bounds visualization.
	Bounds = CreateDefaultSubobject<UBoxComponent>("Bounds");
	Bounds->SetBoxExtent(FVector{ .5f, .5f, .5f }, false);
	Bounds->SetRelativeTransform(FTransform{ FVector{ .5f, .5f, .5f } });
	Bounds->bDrawOnlyIfSelected = true;
	Bounds->SetIsVisualizationComponent(true);
	Bounds->SetCollisionProfileName("NoCollision");
	Bounds->SetCanEverAffectNavigation(false);
	Bounds->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Bounds->SetGenerateOverlapEvents(false);
	Bounds->SetMobility(EComponentMobility::Static);
	Bounds->SetupAttachment(Grid);
#endif
}

void AUEGridVolume::PreRegisterAllComponents()
{
	Super::PreRegisterAllComponents();

	if (!GetActorRotation().IsNearlyZero())
	{
		SetActorRotation(FQuat::Identity);
		UE_LOGFMT(LogUE, Warning, "AUEGridVolume cannot have non-zero rotation, set to zero.");
	}
}

bool AUEGridVolume::NeedsLoadForServer() const
{
	return false;
}

bool AUEGridVolume::IsLevelBoundsRelevant() const
{
	return false;
}
