// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEDemolitionPreviewCursor.h"
#include "Building/UEBuildingSystem.h"
#include "Components/DecalComponent.h"
#include "Components/InputComponent.h"
#include "Engine/OverlapResult.h"
#include "EnhancedInputComponent.h"
#include "Grid/UEGridLibrary.h"
#include "Grid/UEGridPlacementComponent.h"
#include "Grid/UEGridSystem.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Path/UEPathActor.h"
#include "Path/UEPathSystem.h"

namespace
{
	FIntRect GetRect(FIntPoint const Point1, FIntPoint const Point2)
	{
		FIntPoint const Min = Point1.ComponentMin(Point2);
		FIntPoint const Max = Point1.ComponentMax(Point2) + FIntPoint{ 1, 1 };
		return FIntRect{ Min, Max };
	}
} // namespace

AUEDemolitionPreviewCursor::AUEDemolitionPreviewCursor()
{
	GridPlacement->SetGridSize(FIntPoint{ 1, 1 });
	SelectionDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecal"));
	SelectionDecal->SetMobility(EComponentMobility::Type::Movable);
	SelectionDecal->SetFadeScreenSize(0.f);
	{
		bool const bNewAbsoluteLocation = true;
		bool const bNewAbsoluteRotation = true;
		bool const bNewAbsoluteScale = true;
		SelectionDecal->SetAbsolute(bNewAbsoluteLocation, bNewAbsoluteRotation, bNewAbsoluteScale);
	}
	SelectionDecal->SetupAttachment(RootComponent);
	DemolitionPreviewCursorInputMappingContext = nullptr;
	StartSelectingInputAction = nullptr;
	FinishSelectingInputAction = nullptr;
	CancelSelectingInputAction = nullptr;
	PreviousDemolitionPreviev = FIntRect{};
	SelectionStartLocationOnGrid = FIntPoint{};
	SelectionDecalHalfZExtent = 9e4f;
	PreviousDemolitionType = EDemolitionType::NONE;
	bIsSelecting = false;
}

void AUEDemolitionPreviewCursor::BeginPlay()
{
	Super::BeginPlay();

	check(IsValid(GridPlacement) && IsValid(SelectionDecal));
	float const GridCellSize = UUEGridLibrary::GetGridCellSize(this);
	SelectionDecal->DecalSize = FVector{ GridCellSize, GridCellSize, SelectionDecalHalfZExtent * 2.f };
	SelectionStartLocationOnGrid = GridPlacement->GetLocationOnGrid();
	UpdateDemolitionPreview(GetRect(SelectionStartLocationOnGrid, SelectionStartLocationOnGrid), GetDemolitionType(SelectionStartLocationOnGrid));
}

void AUEDemolitionPreviewCursor::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	UpdateDemolitionPreview(FIntRect{}, EDemolitionType::NONE);

	Super::EndPlay(EndPlayReason);
}

void AUEDemolitionPreviewCursor::EnableInput(APlayerController * PlayerController)
{
	Super::EnableInput(PlayerController);

	AddInputMappingContext(PlayerController, DemolitionPreviewCursorInputMappingContext, 1);
}

void AUEDemolitionPreviewCursor::DisableInput(APlayerController * PlayerController)
{
	RemoveInputMappingContext(PlayerController, DemolitionPreviewCursorInputMappingContext);

	Super::DisableInput(PlayerController);
}

void AUEDemolitionPreviewCursor::OnPreviewChanged()
{
	Super::OnPreviewChanged();

	check(IsValid(GridPlacement));
	FIntPoint const CursorLocationOnGrid = GridPlacement->GetLocationOnGrid();
	EDemolitionType NewDemolitionType = PreviousDemolitionType;
	if (!bIsSelecting)
	{
		SelectionStartLocationOnGrid = CursorLocationOnGrid;
		NewDemolitionType = GetDemolitionType(SelectionStartLocationOnGrid);
	}
	UpdateDemolitionPreview(GetRect(SelectionStartLocationOnGrid, CursorLocationOnGrid), NewDemolitionType);
}

void AUEDemolitionPreviewCursor::SetupPlayerInputComponent(UInputComponent * PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	TObjectPtr<UEnhancedInputComponent> EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	check(EnhancedInput && StartSelectingInputAction && FinishSelectingInputAction && CancelSelectingInputAction);

	EnhancedInput->BindAction(StartSelectingInputAction, ETriggerEvent::Triggered, this, &AUEDemolitionPreviewCursor::StartSelecting);
	EnhancedInput->BindAction(FinishSelectingInputAction, ETriggerEvent::Triggered, this, &AUEDemolitionPreviewCursor::FinishSelecting);
	EnhancedInput->BindAction(CancelSelectingInputAction, ETriggerEvent::Triggered, this, &AUEDemolitionPreviewCursor::CancelSelecting);
}

AUEDemolitionPreviewCursor::EDemolitionType AUEDemolitionPreviewCursor::GetDemolitionType(FIntPoint const Coords) const
{
	UUEGridSystem * const GridSystem = UUEGridLibrary::GetGridSystem(this);
	if (IsValid(GridSystem))
	{
		if (!GridSystem->IsCellOccupied(EUEGridLayer::Construction, Coords))
		{
			return EDemolitionType::Foliage;
		}
		if (GridSystem->IsCellOccupied(EUEGridLayer::Road, Coords))
		{
			return EDemolitionType::Road;
		}
		return EDemolitionType::Building;
	}
	return EDemolitionType::NONE;
}

void AUEDemolitionPreviewCursor::UpdateDecalTransform(FIntRect const & Rect)
{
	check(IsValid(SelectionDecal));
	double const GridCellSize = UUEGridLibrary::GetGridCellSize(this);
	FVector2d const RectSize = static_cast<FVector2d>(Rect.Size());
	FVector2d const RectCenterCoords = static_cast<FVector2d>(Rect.Min) + RectSize / 2.;
	FVector const DecalLocation{ GridCellSize * RectCenterCoords.X, GridCellSize * RectCenterCoords.Y, 0. };
	FVector const DecalScale{ RectSize.X, RectSize.Y, 1. };
	FTransform const DecalTransform{ FQuat::Identity, DecalLocation, DecalScale };
	SelectionDecal->SetWorldTransform(DecalTransform);
}

void AUEDemolitionPreviewCursor::UpdateDemolitionPreview(FIntRect && NewDemolitionPreview, EDemolitionType const NewDemolitionType)
{
	TArray<FOverlapResult> OldOverlaps;
	TArray<FOverlapResult> NewOverlaps;
	if (PreviousDemolitionType == EDemolitionType::Foliage)
	{
		OldOverlaps = GetFoliageOverlaps(PreviousDemolitionPreviev);
	}
	if (NewDemolitionType == EDemolitionType::Foliage)
	{
		NewOverlaps = GetFoliageOverlaps(NewDemolitionPreview);
	}
	HideAndShowOverlappedFoliage(OldOverlaps, NewOverlaps);
	UpdateDecalTransform(NewDemolitionPreview);
	PreviousDemolitionPreviev = MoveTemp(NewDemolitionPreview);
	PreviousDemolitionType = NewDemolitionType;
}

void AUEDemolitionPreviewCursor::StartSelecting()
{
	bIsSelecting = true;
}

void AUEDemolitionPreviewCursor::FinishSelecting()
{
	if (!bIsSelecting)
	{
		return;
	}
	bIsSelecting = false;
	switch (PreviousDemolitionType)
	{
		case EDemolitionType::Foliage:
		{
			RemoveOverlappedFoliage(GetFoliageOverlaps(PreviousDemolitionPreviev));
			break;
		}
		case EDemolitionType::Building:
		{
			if (TObjectPtr<UWorld> const World = GetWorld(); IsValid(World))
			{
				if (TObjectPtr<UUEBuildingSystem> const BuildingSystem = World->GetSubsystem<UUEBuildingSystem>())
				{
					BuildingSystem->GetOverlappedBuildingsAsync(PreviousDemolitionPreviev, [](TArray<TObjectPtr<AActor>> && Buildings) {
							AsyncTask(ENamedThreads::GameThread, [Buildings = MoveTemp(Buildings)]() {	
									for (TObjectPtr<AActor> Building : Buildings)
									{
										Building->Destroy();
									}
								});
						});
				}
			}
			break;
		}
		case EDemolitionType::Road:
		{
			if (TObjectPtr<UWorld> const World = GetWorld(); IsValid(World))
			{
				if (TObjectPtr<UUEPathSystem> const PathSystem = World->GetSubsystem<UUEPathSystem>())
				{
					for (TObjectPtr<AUEPathActor> const PathActor : PathSystem->GetPathActors(EUEPathGraph::Road))
					{
						if (IsValid(PathActor))
						{
							PathActor->RemovePath(PreviousDemolitionPreviev);
						}
					}
				}
			}
			break;
		}
		case EDemolitionType::NONE:
		{
			break;
		}
	}
	PreviousDemolitionPreviev = FIntRect{};
	PreviousDemolitionType = EDemolitionType::NONE;
	OnPreviewChanged();
}

void AUEDemolitionPreviewCursor::CancelSelecting()
{
	if (bIsSelecting)
	{
		bIsSelecting = false;
		OnPreviewChanged();
	}
}
