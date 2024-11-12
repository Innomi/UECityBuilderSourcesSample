// Fill out your copyright notice in the Description page of Project Settings.

#include "Path/UEPathPreviewCursor.h"
#include "AStar/UEGridPathFilter.h"
#include "AStar/UEGridToGraphAdapter.h"
#include "Common/UEOverlapResultUtilities.h"
#include "Engine/OverlapResult.h"
#include "EnhancedInputComponent.h"
#include "GraphAStar.h"
#include "Grid/UEGridLibrary.h"
#include "Grid/UEGridPlacementComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Path/UEPathActor.h"
#include "Path/UEPathSystem.h"

namespace
{
	template <typename AllocatorType1, typename AllocatorType2>
	FORCEINLINE_DEBUGGABLE void AppendUniqueOverlaps(TArray<FOverlapResult, AllocatorType1> & InOutOverlaps, TArray<FOverlapResult, AllocatorType2> const & NewOverlaps)
	{
		for (FOverlapResult const & NewOverlap : NewOverlaps)
		{
			if (InOutOverlaps.IndexOfByPredicate(FOverlapResultCompare(NewOverlap)) == INDEX_NONE)
			{
				InOutOverlaps.Add(NewOverlap);
			}
		}
	}
} // namespace

AUEPathPreviewCursor::AUEPathPreviewCursor()
{
	GridPlacement->SetGridSize(FIntPoint{ 1, 1 });
	PreviousPathPreview = TArray<FIntPoint>{};
	PathActor = nullptr;
	PathPreviewCursorInputMappingContext = nullptr;
	StartPathPlacingInputAction = nullptr;
	FinishPathPlacingInputAction = nullptr;
	CancelPathPlacingInputAction = nullptr;
	PathStartLocationOnGrid = FIntPoint{};
	bIsGoingAlongXFirst = false;
	bIsPlacingPath = false;
}

void AUEPathPreviewCursor::BeginPlay()
{
	Super::BeginPlay();
	
	check(IsValid(GridPlacement));
	PathStartLocationOnGrid = GridPlacement->GetLocationOnGrid();
	UpdatePathPreview(GetPath(PathStartLocationOnGrid, PathStartLocationOnGrid));
}

void AUEPathPreviewCursor::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	UpdatePathPreview(TArray<FIntPoint>{});

	Super::EndPlay(EndPlayReason);
}

void AUEPathPreviewCursor::EnableInput(APlayerController * PlayerController)
{
	Super::EnableInput(PlayerController);

	AddInputMappingContext(PlayerController, PathPreviewCursorInputMappingContext, 1);
}

void AUEPathPreviewCursor::DisableInput(APlayerController * PlayerController)
{
	RemoveInputMappingContext(PlayerController, PathPreviewCursorInputMappingContext);

	Super::DisableInput(PlayerController);
}

void AUEPathPreviewCursor::SetupFor(TSubclassOf<AUEPathActor> const PathActorClass)
{
	if (IsValid(PathActor))
	{
		PathActor->CancelPreview();
	}
	PathActor = nullptr;
	if (TObjectPtr<UWorld> const World = GetWorld())
	{
		if (TObjectPtr<UUEPathSystem> const PathSystem = World->GetSubsystem<UUEPathSystem>())
		{
			PathActor = PathSystem->GetPathActor(PathActorClass);
		}
	}
}

void AUEPathPreviewCursor::OnPreviewChanged()
{
	Super::OnPreviewChanged();

	check(IsValid(GridPlacement));
	FIntPoint const CursorLocationOnGrid = GridPlacement->GetLocationOnGrid();
	if (bIsPlacingPath)
	{
		FIntPoint const CursorAndPathStartLocationsDiff = CursorLocationOnGrid - PathStartLocationOnGrid;
		if (CursorAndPathStartLocationsDiff.SizeSquared() == 1)
		{
			bIsGoingAlongXFirst = (CursorAndPathStartLocationsDiff.X == 0);
		}
	}
	else
	{
		PathStartLocationOnGrid = CursorLocationOnGrid;
	}
	UpdatePathPreview(GetPath(PathStartLocationOnGrid, CursorLocationOnGrid));
}

void AUEPathPreviewCursor::SetupPlayerInputComponent(UInputComponent * PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	TObjectPtr<UEnhancedInputComponent> EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	check(EnhancedInput && StartPathPlacingInputAction && FinishPathPlacingInputAction && CancelPathPlacingInputAction && ChangePriorityAxisInputAction);

	EnhancedInput->BindAction(StartPathPlacingInputAction, ETriggerEvent::Triggered, this, &AUEPathPreviewCursor::StartPathPlacing);
	EnhancedInput->BindAction(FinishPathPlacingInputAction, ETriggerEvent::Triggered, this, &AUEPathPreviewCursor::FinishPathPlacing);
	EnhancedInput->BindAction(CancelPathPlacingInputAction, ETriggerEvent::Triggered, this, &AUEPathPreviewCursor::CancelPathPlacing);
	EnhancedInput->BindAction(ChangePriorityAxisInputAction, ETriggerEvent::Triggered, this, &AUEPathPreviewCursor::ChangePriorityAxis);
}

TArray<FOverlapResult> AUEPathPreviewCursor::GetFoliageOverlaps(TArray<FIntPoint> const & Path) const
{
	TArray<FOverlapResult> Overlaps;
	for (size_t PathIndex = 0; PathIndex < Path.Num();)
	{
		size_t const FromIndex = PathIndex;
		++PathIndex;
		if (PathIndex < Path.Num() && (Path[FromIndex] - Path[PathIndex]).SizeSquared() == 1)
		{
			EUEGridDirection const Direction = FUEGridDirectionUtil::GetDirection(Path[FromIndex], Path[PathIndex]);
			++PathIndex;
			while (PathIndex < Path.Num()
				&& Direction == FUEGridDirectionUtil::GetDirection(Path[PathIndex - 1], Path[PathIndex])
				&& (Path[PathIndex - 1] - Path[PathIndex]).SizeSquared() == 1)
			{
				++PathIndex;
			}
		}
		{
			FIntPoint const FromCoords = Path[FromIndex];
			FIntPoint const ToCoords = Path[PathIndex - 1];
			FIntPoint const Min = FromCoords.ComponentMin(ToCoords);
			FIntPoint const Max = FromCoords.ComponentMax(ToCoords) + FIntPoint{ 1, 1 };
			AppendUniqueOverlaps(Overlaps, Super::GetFoliageOverlaps(FIntRect{ Min, Max }));
		}
	}
	return Overlaps;
}

TArray<FIntPoint> AUEPathPreviewCursor::GetPath(FIntPoint const PathStartLocation, FIntPoint const PathEndLocation) const
{
	TObjectPtr<UUEGridSystem const> const GridSystem = UUEGridLibrary::GetGridSystem(this);
	if (UNLIKELY(!GridSystem || !IsValid(PathActor)))
	{
		return TArray<FIntPoint>{};
	}
	FUEGridToGraphAdapter GridToGraphAdapter(GridSystem, PathActor->GetPathRelatedGridLayer());
	// FGraphAStar returns empty path when start equals end.
	if (PathStartLocation == PathEndLocation)
	{
		if (GridToGraphAdapter.IsValidRef(PathStartLocation))
		{
			return TArray<FIntPoint>{ PathStartLocation };
		}
		else
		{
			return TArray<FIntPoint>{};
		}
	}
	FGraphAStar<FUEGridToGraphAdapter> AStar(GridToGraphAdapter);
	double const HeuristicScale = 1.00001;
	FVector2D const AxiswiseHeuristicScale = bIsGoingAlongXFirst ? FVector2D{ 1., 1.00001 } : FVector2D{ 1.00001, 1. };
	FUEGridPathFilter GridPathFilter(HeuristicScale, AxiswiseHeuristicScale);
	TArray<FIntPoint> Path;
	Path.Reserve(GridPathFilter.GetMaxSearchNodes());
	AStar.FindPath(PathStartLocation, PathEndLocation, GridPathFilter, Path);
	return Path;
}

void AUEPathPreviewCursor::UpdatePathPreview(TArray<FIntPoint> && NewPathPreview)
{
	HideAndShowOverlappedFoliage(GetFoliageOverlaps(PreviousPathPreview), GetFoliageOverlaps(NewPathPreview));
	if (IsValid(PathActor))
	{
		PathActor->CreatePreview(NewPathPreview);
	}
	PreviousPathPreview = MoveTemp(NewPathPreview);
}

void AUEPathPreviewCursor::StartPathPlacing()
{
	bIsPlacingPath = true;
}

void AUEPathPreviewCursor::FinishPathPlacing()
{
	if (!bIsPlacingPath)
	{
		return;
	}
	bIsPlacingPath = false;
	if (IsValid(PathActor))
	{
		PathActor->CommitPreview();
	}
	RemoveOverlappedFoliage(GetFoliageOverlaps(PreviousPathPreview));
	PreviousPathPreview.Empty();
	OnPreviewChanged();
}

void AUEPathPreviewCursor::CancelPathPlacing()
{
	if (bIsPlacingPath)
	{
		bIsPlacingPath = false;
		OnPreviewChanged();
	}
}

void AUEPathPreviewCursor::ChangePriorityAxis()
{
	bIsGoingAlongXFirst = static_cast<uint8>(!bIsGoingAlongXFirst);
	OnPreviewChanged();
}