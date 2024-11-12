// Fill out your copyright notice in the Description page of Project Settings.

#include "Building/UEBuildingPreviewCursor.h"
#include "Building/UEBuildingActor.h"
#include "Components/InputComponent.h"
#include "Engine/OverlapResult.h"
#include "EnhancedInputComponent.h"
#include "Grid/UEGridPlacementComponent.h"
#include "Grid/UEGridSystem.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AUEBuildingPreviewCursor::AUEBuildingPreviewCursor()
{
	PreviewBuildingClass = nullptr;
	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(GridPlacement, UUEGridPlacementComponent::GridCenterSocketName);
	PreviewMesh->SetMobility(EComponentMobility::Type::Movable);
	PreviewMesh->SetCollisionProfileName("OverlapAllDynamic");
	BuildingPreviewCursorInputMappingContext = nullptr;
	TryToPlaceBuildingInputAction = nullptr;
	RotateClockwiseInputAction = nullptr;
	RotateCounterclockwiseInputAction = nullptr;
}

void AUEBuildingPreviewCursor::BeginPlay()
{
	Super::BeginPlay();

	PreviousPreviewRotation = GetPreviewRotation();
	TArray<FOverlapResult> NewOverlaps = GetFoliageOverlaps(PreviousPreviewLocation, PreviousPreviewRotation);
	HideAndShowOverlappedFoliage(TArray<FOverlapResult>(), NewOverlaps);
}

void AUEBuildingPreviewCursor::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	TArray<FOverlapResult> OldOverlaps = GetFoliageOverlaps(PreviousPreviewLocation, PreviousPreviewRotation);
	HideAndShowOverlappedFoliage(OldOverlaps, TArray<FOverlapResult>());

	Super::EndPlay(EndPlayReason);
}

void AUEBuildingPreviewCursor::EnableInput(APlayerController * PlayerController)
{
	Super::EnableInput(PlayerController);

	AddInputMappingContext(PlayerController, BuildingPreviewCursorInputMappingContext);
}

void AUEBuildingPreviewCursor::DisableInput(APlayerController * PlayerController)
{
	RemoveInputMappingContext(PlayerController, BuildingPreviewCursorInputMappingContext);

	Super::DisableInput(PlayerController);
}

void AUEBuildingPreviewCursor::SetupFor(TSubclassOf<AUEBuildingActor> const BuildingActorClass)
{
	if (!IsValid(BuildingActorClass))
	{
		return;
	}

	PreviewBuildingClass = BuildingActorClass;
	if (TObjectPtr<AActor const> BuildingCDO = BuildingActorClass->GetDefaultObject<AActor const>(); IsValid(BuildingCDO))
	{
		TObjectPtr<UStaticMeshComponent> StaticMeshComponent = BuildingCDO->FindComponentByClass<UStaticMeshComponent>();
		if (IsValid(StaticMeshComponent))
		{
			check(IsValid(PreviewMesh));
			PreviewMesh->SetStaticMesh(StaticMeshComponent->GetStaticMesh());
		}
		TObjectPtr<UUEGridPlacementComponent> CDOUEGridPlacementComponent = BuildingCDO->FindComponentByClass<UUEGridPlacementComponent>();
		if (IsValid(CDOUEGridPlacementComponent))
		{
			check(IsValid(GridPlacement));
			GridPlacement->SetGridSize(CDOUEGridPlacementComponent->GetGridSize());
		}
	}
}

bool AUEBuildingPreviewCursor::IsCursorSetup() const
{
	return IsValid(BoundPlayerController) && IsValid(PreviewBuildingClass);
}

TArray<FOverlapResult> AUEBuildingPreviewCursor::GetFoliageOverlaps(FVector const & Location, FQuat const & Rotation) const
{
	check(IsValid(PreviewMesh));
	check(PreviewMesh->GetRelativeLocation().IsNearlyZero() && PreviewMesh->GetRelativeRotation().IsNearlyZero());
	TArray<FOverlapResult> Overlaps;
	PreviewMesh->ComponentOverlapMulti(Overlaps, PreviewMesh->GetWorld(), Location, Rotation, FoliageOverlapChannel);
	return Overlaps;
}

void AUEBuildingPreviewCursor::OnPreviewChanged()
{
	Super::OnPreviewChanged();

	TArray<FOverlapResult> OldOverlaps = GetFoliageOverlaps(PreviousPreviewLocation, PreviousPreviewRotation);
	TArray<FOverlapResult> NewOverlaps = GetFoliageOverlaps(GetPreviewLocation(), GetPreviewRotation());
	HideAndShowOverlappedFoliage(OldOverlaps, NewOverlaps);
}

void AUEBuildingPreviewCursor::SetupPlayerInputComponent(UInputComponent * PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	TObjectPtr<UEnhancedInputComponent> EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	check(EnhancedInput && TryToPlaceBuildingInputAction && RotateClockwiseInputAction && RotateCounterclockwiseInputAction);

	EnhancedInput->BindAction(TryToPlaceBuildingInputAction, ETriggerEvent::Triggered, this, &AUEBuildingPreviewCursor::TryToPlaceBuilding);
	EnhancedInput->BindAction(RotateClockwiseInputAction, ETriggerEvent::Triggered, this, &AUEBuildingPreviewCursor::RotateClockwise);
	EnhancedInput->BindAction(RotateCounterclockwiseInputAction, ETriggerEvent::Triggered, this, &AUEBuildingPreviewCursor::RotateCounterclockwise);
}

void AUEBuildingPreviewCursor::TryToPlaceBuilding()
{
	FIntPoint const LocationOnGrid{ GridPlacement->GetLocationOnGrid() };
	FIntRect const GridRect{ GridPlacement->GetGridRect(LocationOnGrid) };
	// TODO: make api to check for nature obstacle more convinient.
	if (!IsCursorSetup() || !GridPlacement->CanBePlacedOnGrid(GridRect) || !GridPlacement->CanBePlacedOnGrid(EUEGridLayer::NatureObstacle, GridRect))
	{
		return;
	}

	if (TObjectPtr<UWorld> World = GetWorld(); IsValid(World))
	{
		RemoveOverlappedFoliage(GetFoliageOverlaps(GetPreviewLocation(), GetPreviewRotation()));
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<AUEGridPlacedActor>(PreviewBuildingClass, GetTransform(), SpawnParameters);
	}
}

void AUEBuildingPreviewCursor::RotateAndGridSnap(double const DeltaYaw)
{
	FRotator const NewCursorRotation = GetActorRotation().Add(0, DeltaYaw, 0).GridSnap({ 0, 90., 0 });
	FVector const GridCenterLocation = GetPreviewLocation();
	SetActorRotation(NewCursorRotation);
	SetCursorLocation(GridCenterLocation);
	OnPreviewChanged();
	PreviousPreviewRotation = GetPreviewRotation();
}

void AUEBuildingPreviewCursor::RotateClockwise()
{
	if (!IsCursorSetup())
	{
		return;
	}

	RotateAndGridSnap(90.);
}

void AUEBuildingPreviewCursor::RotateCounterclockwise()
{
	if (!IsCursorSetup())
	{
		return;
	}

	RotateAndGridSnap(-90.);
}