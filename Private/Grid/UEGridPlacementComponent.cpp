// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEGridPlacementComponent.h"
#include "Common/UELog.h"
#include "ComponentReregisterContext.h"
#include "Grid/UEGridLibrary.h"
#include "Grid/UEGridSystem.h"

FName const UUEGridPlacementComponent::GridCenterSocketName("GridCenter");

UUEGridPlacementComponent::UUEGridPlacementComponent()
{
	GridLayerToRegisterOn = EUEGridLayer::Construction;
	GridSize = FIntPoint::ZeroValue;
	bIsRegisteredOnGrid = false;
	bShouldBeRegisteredOnGrid = true;
}

void UUEGridPlacementComponent::OnRegister()
{
	Super::OnRegister();

	// Movable component can't be placed on grid.
	if (Mobility != EComponentMobility::Type::Movable && ShouldBeRegisteredOnGrid())
	{
		FIntPoint const LocationOnGrid{ GetLocationOnGrid() };
		FIntRect const GridRect{ GetGridRect(LocationOnGrid) };
		if (CanBePlacedOnGrid(GridRect))
		{
			bool const bIsOccupied = true;
			SetOccupationOnGrid(GridLayerToRegisterOn, GridRect, bIsOccupied);
			bIsRegisteredOnGrid = true;
		}
		else
		{
			TObjectPtr<AActor> Owner = GetOwner();
			FString const ActorName = Owner ? Owner->GetName() : TEXT("UndefinedActor");
			UE_LOGFMT(LogUE, Warning, "Registering {0} at X: {1} Y: {2} on grid failed due to already occupied grid cells.", ActorName, LocationOnGrid.X, LocationOnGrid.Y);
		}
	}
}

void UUEGridPlacementComponent::OnUnregister()
{
	if (bIsRegisteredOnGrid)
	{
		FIntRect const GridRect{ GetGridRect(GetLocationOnGrid()) };
		bool const bIsOccupied = false;
		SetOccupationOnGrid(GridLayerToRegisterOn, GridRect, bIsOccupied);
		bIsRegisteredOnGrid = false;
	}

	Super::OnUnregister();
}

bool UUEGridPlacementComponent::HasAnySockets() const
{
	return true;
}

FTransform UUEGridPlacementComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	double const HalfGridCellSize = UUEGridLibrary::GetGridCellSize(this) / 2.;
	FVector const SocketOffset = { HalfGridCellSize * GridSize.X,  HalfGridCellSize * GridSize.Y, 0. };
	FTransform const RelativeSocketTransform(SocketOffset);

	switch (TransformSpace)
	{
	case RTS_World:
	{
		return RelativeSocketTransform * GetComponentTransform();
	}
	case RTS_Actor:
	{
		if (AActor const * Actor = GetOwner())
		{
			FTransform const SocketTransform = RelativeSocketTransform * GetComponentTransform();
			return SocketTransform.GetRelativeTransform(Actor->GetTransform());
		}
		break;
	}
	case RTS_Component:
	{
		return RelativeSocketTransform;
	}
	}
	return RelativeSocketTransform;
}

void UUEGridPlacementComponent::QuerySupportedSockets(TArray<FComponentSocketDescription> & OutSockets) const
{
	new (OutSockets) FComponentSocketDescription(GridCenterSocketName, EComponentSocketType::Socket);
}

FIntPoint UUEGridPlacementComponent::GetGridSize() const
{
	return GridSize;
}

void UUEGridPlacementComponent::SetGridSize(FIntPoint const InGridSize)
{
	if (InGridSize.X >= 0 && InGridSize.Y >= 0 && InGridSize != GridSize)
	{
		if (bIsRegisteredOnGrid)
		{
			FComponentReregisterContext ReregisterContext(this);
			GridSize = InGridSize;
		}
		else
		{
			GridSize = InGridSize;
		}
	}
}

FIntPoint UUEGridPlacementComponent::GetLocationOnGrid() const
{
	FVector const ComponentLocation = GetComponentLocation();
	return UUEGridLibrary::GetGridCellCoords(this, FVector2D{ ComponentLocation });
}

FIntRect UUEGridPlacementComponent::GetGridRect(FIntPoint const LocationOnGrid) const
{
	FRotator SnappedRotation = GetComponentRotation().GridSnap({ 0, 90, 0 });
	int32 MinX = LocationOnGrid.X;
	int32 MaxX = LocationOnGrid.X;
	int32 MinY = LocationOnGrid.Y;
	int32 MaxY = LocationOnGrid.Y;
	if (SnappedRotation.Equals({ 0, 0, 0 }))
	{
		MaxX += GridSize.X;
		MaxY += GridSize.Y;
	}
	else if (SnappedRotation.Equals({ 0, 90, 0 }))
	{
		MaxY += GridSize.X;
		MinX -= GridSize.Y;
	}
	else if (SnappedRotation.Equals({ 0, 180, 0 }))
	{
		MinX -= GridSize.X;
		MinY -= GridSize.Y;
	}
	else
	{
		MinY -= GridSize.X;
		MaxX += GridSize.Y;
	}
	return { MinX, MinY, MaxX, MaxY };
}

EUEGridLayer UUEGridPlacementComponent::GetLayerToRegisterOn() const
{
	return GridLayerToRegisterOn;
}

void UUEGridPlacementComponent::SetLayerToRegisterOn(EUEGridLayer const GridLayer)
{
	if (GridLayer != GridLayerToRegisterOn)
	{
		if (bIsRegisteredOnGrid)
		{
			FComponentReregisterContext ReregisterContext(this);
			GridLayerToRegisterOn = GridLayer;
		}
		else
		{
			GridLayerToRegisterOn = GridLayer;
		}
	}
}

bool UUEGridPlacementComponent::IsRegisteredOnGrid() const
{
	return bIsRegisteredOnGrid;
}

bool UUEGridPlacementComponent::ShouldBeRegisteredOnGrid() const
{
	return bShouldBeRegisteredOnGrid;
}

void UUEGridPlacementComponent::SetShouldBeRegisteredOnGrid(bool const bInShouldBeRegisteredOnGrid)
{
	if (bInShouldBeRegisteredOnGrid != bShouldBeRegisteredOnGrid)
	{
		FComponentReregisterContext ReregisterContext(this);
		bShouldBeRegisteredOnGrid = bInShouldBeRegisteredOnGrid;
	}
}

bool UUEGridPlacementComponent::CanBePlacedOnGrid(FIntRect const & Rect) const
{
	return CanBePlacedOnGrid(GridLayerToRegisterOn, Rect);
}

bool UUEGridPlacementComponent::CanBePlacedOnGrid(EUEGridLayer const GridLayer, FIntRect const & Rect) const
{
	if (UUEGridSystem * const GridSystem = UUEGridLibrary::GetGridSystem(this); GridSystem && UUEGridLibrary::IsInSingleGridComponent(this, Rect))
	{
		return !GridSystem->HasOccupiedCell(GridLayer, Rect);
	}
	return false;
}

FVector UUEGridPlacementComponent::GridSnapLocation(FVector const & InLocation) const
{
	return { UUEGridLibrary::GridSnap(this, InLocation.X), UUEGridLibrary::GridSnap(this, InLocation.Y), InLocation.Z };
}

void UUEGridPlacementComponent::SetOccupationOnGrid(EUEGridLayer const GridLayer, FIntPoint const Coords, bool const bIsOccupied) const
{
	if (UUEGridSystem * const GridSystem = UUEGridLibrary::GetGridSystem(this))
	{
		GridSystem->SetCellState(GridLayer, Coords, bIsOccupied);
	}
}

void UUEGridPlacementComponent::SetOccupationOnGrid(EUEGridLayer const GridLayer, FIntRect const & Rect, bool const bIsOccupied) const
{
	if (UUEGridSystem * const GridSystem = UUEGridLibrary::GetGridSystem(this))
	{
		GridSystem->SetCellsState(GridLayer, Rect, bIsOccupied);
	}
}
