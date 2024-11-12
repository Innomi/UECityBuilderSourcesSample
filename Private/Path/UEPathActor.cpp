// Fill out your copyright notice in the Description page of Project Settings.

#include "Path/UEPathActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Grid/UEGridDirection.h"
#include "Grid/UEGridLibrary.h"
#include "Grid/UEGridSystem.h"
#include "Path/UEPathPlacementComponent.h"
#include "Path/UEPathSystem.h"

namespace
{
	TArray<int32> RemoveAndAddInstances(TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISMComponent, TArray<int32> const & ReverseSortedInstancesToRemove, TArray<FTransform> const & TransformsToAdd, bool const bShouldReturnIndices = false)
	{
		check(IsValid(HISMComponent));
		TArray<int32> AddedInstances;
		bool const bPreviousAutoRebuildTreeOnInstanceChanges = HISMComponent->bAutoRebuildTreeOnInstanceChanges;
		HISMComponent->bAutoRebuildTreeOnInstanceChanges = false;
		{
			bool const bInstanceArrayAlreadySortedInReverseOrder = true;
			HISMComponent->RemoveInstances(ReverseSortedInstancesToRemove, bInstanceArrayAlreadySortedInReverseOrder);
		}
		{
			bool const bWorldSpace = true;
			AddedInstances = HISMComponent->AddInstances(TransformsToAdd, bShouldReturnIndices, bWorldSpace);
		}
		{
			bool const bAsync = false;
			bool const bForceUpdate = false;
			HISMComponent->BuildTreeIfOutdated(bAsync, bForceUpdate);
		}
		HISMComponent->bAutoRebuildTreeOnInstanceChanges = bPreviousAutoRebuildTreeOnInstanceChanges;
		return AddedInstances;
	}
} // namespace

AUEPathActor::AUEPathActor()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("RootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);

	PathPlacement = CreateDefaultSubobject<UUEPathPlacementComponent>("PathPlacement");
	PathPlacement->SetupAttachment(RootComponent);
	PathPlacement->SetMobility(EComponentMobility::Static);

	ZeroConnectionsPathHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>("ZeroConnectionsPathHISM");
	PathHISM[static_cast<uint8>(EPathMesh::ZeroConnections)] = ZeroConnectionsPathHISM;
	TwoOppositeConnectionsPathHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>("TwoOppositeConnectionsPathHISM");
	PathHISM[static_cast<uint8>(EPathMesh::TwoOppositeConnections)] = TwoOppositeConnectionsPathHISM;
	TwoCloseConnectionsPathHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>("TwoCloseConnectionsPathHISM");
	PathHISM[static_cast<uint8>(EPathMesh::TwoCloseConnections)] = TwoCloseConnectionsPathHISM;
	ThreeConnectionsPathHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>("ThreeConnectionsPathHISM");
	PathHISM[static_cast<uint8>(EPathMesh::ThreeConnections)] = ThreeConnectionsPathHISM;
	FourConnectionsPathHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>("FourConnectionsPathHISM");
	PathHISM[static_cast<uint8>(EPathMesh::FourConnections)] = FourConnectionsPathHISM;

	for (TObjectPtr<UHierarchicalInstancedStaticMeshComponent> PathHISMComponent : PathHISM)
	{
		PathHISMComponent->SetCastShadow(false);
		PathHISMComponent->SetMobility(EComponentMobility::Static);
		PathHISMComponent->SetCollisionProfileName("NoCollision");
		PathHISMComponent->SetupAttachment(RootComponent);
	}

	GroundTraceHalfLength = 9e4f;
	GroundTraceChannel = ECollisionChannel::ECC_WorldStatic;
}

void AUEPathActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	check(IsValid(PathPlacement));
	if (TObjectPtr<UUEPathSystem> const PathSystem = PathPlacement->GetPathSystem())
	{
		PathSystem->RegisterActor(this, PathPlacement->GetPathGraphToRegister());
	}
}

void AUEPathActor::UnregisterAllComponents(bool const bForReregister)
{
	if (IsValid(PathPlacement))
	{
		if (TObjectPtr<UUEPathSystem> const PathSystem = PathPlacement->GetPathSystem())
		{
			PathSystem->UnregisterActor(this, PathPlacement->GetPathGraphToRegister());
		}
	}

	Super::UnregisterAllComponents(bForReregister);
}

void AUEPathActor::CreatePreview(TArray<FIntPoint> const & Path)
{
	check(IsValid(PathPlacement));

	UUEGridSystem const * const GridSystem = UUEGridLibrary::GetGridSystem(this);
	if (UNLIKELY(!GridSystem))
	{
		return;
	}

	CancelPreview();
	PathPreview = Path;

	TMap<FIntPoint, FAdjacentCellsOccupation> AdjacentCellsOccupation;
	AdjacentCellsOccupation.Reserve(PathPreview.Num() * 5);
	for (FIntPoint const & Coords : PathPreview)
	{
		if (UNLIKELY(!GridSystem->IsInGrid(Coords)) || PathPlacement->IsPathAt(Coords))
		{
			continue;
		}
		GetAdjacentOccupiedCells(Coords, AdjacentCellsOccupation.FindOrAdd(Coords));
		for (EUEGridDirection const GridDirection : TEnumRange<EUEGridDirection>())
		{
			FIntPoint const AdjacentCoords = FUEGridDirectionUtil::GetAdjacentCoordsUnsafe(Coords, GridDirection);
			if (!GridSystem->IsInGrid(AdjacentCoords))
			{
				continue;
			}
			uint8 const UintOppositeDirection = static_cast<uint8>(FUEGridDirectionUtil::GetOppositeDirectionUnsafe(GridDirection));
			AdjacentCellsOccupation.FindOrAdd(AdjacentCoords).NESW[UintOppositeDirection] = true;
			if (PathPlacement->IsPathAt(AdjacentCoords))
			{
				GetAdjacentOccupiedCells(AdjacentCoords, AdjacentCellsOccupation[AdjacentCoords]);
			}
		}
	}

	// Now there are AdjacentCellsOccupations for not occupied coords from PathPreview and occupied and not occupied adjacent to them coords.
	// Weeding out of not occupied adjacent to PathPreview coords is needed. Just ignore them during meshes placing.
	TArray<int32> InstancesToRemove[PathMeshesNum];
	TArray<FTransform> PreviewTransforms[PathMeshesNum];
	for (TArray<FTransform> & PreviewTransformsForSpecificMesh : PreviewTransforms)
	{
		PreviewTransformsForSpecificMesh.Reserve(PathPreview.Num());
	}
	TFunction<void (FIntPoint const)> GetPreviewTransform = [this, &PreviewTransforms, &AdjacentCellsOccupation](FIntPoint const Coords)
		{
			EPathMesh PathMesh;
			FTransform PreviewTransform{ ENoInit() };
			GetPathMeshAndTransform(Coords, AdjacentCellsOccupation[Coords], PathMesh, PreviewTransform);
			uint8 const UintPathMesh = static_cast<uint8>(PathMesh);
			if (LIKELY(UintPathMesh < PathMeshesNum))
			{
				PreviewTransforms[UintPathMesh].Emplace(MoveTemp(PreviewTransform));
			}
		};
	// AdjacentCellsOccupation Contains and Remove are used to prevent multiple processing of same coords.
	for (FIntPoint const & Coords : PathPreview)
	{
		if (UNLIKELY(!GridSystem->IsInGrid(Coords)) || PathPlacement->IsPathAt(Coords) || !AdjacentCellsOccupation.Contains(Coords))
		{
			continue;
		}
		GetPreviewTransform(Coords);
		AdjacentCellsOccupation.Remove(Coords);
		for (FIntPoint const AdjacentCoords : FUEGridDirectionUtil::GetAdjacentCoords(Coords))
		{
			if (PathPlacement->IsPathAt(AdjacentCoords) && AdjacentCellsOccupation.Contains(AdjacentCoords))
			{
				EPathMesh PathMesh;
				int32 InstanceIndex;
				GetPathMeshAndInstanceIndex(AdjacentCoords, PathMesh, InstanceIndex);
				uint8 const UintPathMesh = static_cast<uint8>(PathMesh);
				if (UintPathMesh < PathMeshesNum)
				{
					GetPreviewTransform(AdjacentCoords);
					InstancesToRemove[UintPathMesh].Emplace(InstanceIndex);
					PrePreviewTransforms[UintPathMesh].AddUninitialized();
					bool const bWorldSpace = true;
					PathHISM[UintPathMesh]->GetInstanceTransform(InstanceIndex, PrePreviewTransforms[UintPathMesh].Last(), bWorldSpace);
				}
				AdjacentCellsOccupation.Remove(AdjacentCoords);
			}
		}
	}

	for (uint8 UintPathMesh = 0; UintPathMesh < PathMeshesNum; ++UintPathMesh)
	{
		InstancesToRemove[UintPathMesh].Sort(TGreater<int32>());
		bool const bShouldReturnIndices = true;
		PreviewInstances[UintPathMesh] = RemoveAndAddInstances(PathHISM[UintPathMesh], InstancesToRemove[UintPathMesh], PreviewTransforms[UintPathMesh], bShouldReturnIndices);
	}
}

void AUEPathActor::CancelPreview()
{
	for (uint8 UintPathMesh = 0; UintPathMesh < PathMeshesNum; ++UintPathMesh)
	{
		// PreviewInstances are filled with sorted with < indices of meshes added with AddInstances in CreatePreview.
		Algo::Reverse(PreviewInstances[UintPathMesh]);
		RemoveAndAddInstances(PathHISM[UintPathMesh], PreviewInstances[UintPathMesh], PrePreviewTransforms[UintPathMesh]);
	}
	ClearPreviewData();
}

void AUEPathActor::CommitPreview()
{
	check(IsValid(PathPlacement));
	for (size_t PathPreviewIndex = 0; PathPreviewIndex < PathPreview.Num();)
	{
		while (PathPreviewIndex < PathPreview.Num() && PathPlacement->IsPathAt(PathPreview[PathPreviewIndex]))
		{
			++PathPreviewIndex;
		}
		size_t const FromIndex = PathPreviewIndex;
		++PathPreviewIndex;
		if (PathPreviewIndex < PathPreview.Num()
			&& !PathPlacement->IsPathAt(PathPreview[PathPreviewIndex])
			&& (PathPreview[FromIndex] - PathPreview[PathPreviewIndex]).SizeSquared() == 1)
		{
			EUEGridDirection const Direction = FUEGridDirectionUtil::GetDirection(PathPreview[FromIndex], PathPreview[PathPreviewIndex]);
			++PathPreviewIndex;
			while (PathPreviewIndex < PathPreview.Num()
				&& !PathPlacement->IsPathAt(PathPreview[PathPreviewIndex])
				&& Direction == FUEGridDirectionUtil::GetDirection(PathPreview[PathPreviewIndex - 1], PathPreview[PathPreviewIndex])
				&& (PathPreview[PathPreviewIndex - 1] - PathPreview[PathPreviewIndex]).SizeSquared() == 1)
			{
				++PathPreviewIndex;
			}
		}
		if (FromIndex < PathPreview.Num())
		{
			PathPlacement->RegisterPath(PathPreview[FromIndex], PathPreview[PathPreviewIndex - 1]);
		}
	}
	ClearPreviewData();
}

void AUEPathActor::RemovePath(FIntRect const & Rect)
{
	check(IsValid(PathPlacement));
	CancelPreview();
	if (Rect.IsEmpty() || !UUEGridLibrary::IsInSingleGridComponent(this, Rect))
	{
		return;
	}

	TArray<int32> InstancesToRemove[PathMeshesNum];
	TArray<FTransform> TransformsToAdd[PathMeshesNum];
	{
		// Firstly, remove all adjacent instances to later restore needed.
		FVector2D const Min2D = UUEGridLibrary::GetGridCellCenter(this, Rect.Min - FIntPoint{ 1, 1 });
		FVector2D const Max2D = UUEGridLibrary::GetGridCellCenter(this, Rect.Max);
		FBox const Box{ FVector{ Min2D, -GroundTraceHalfLength }, FVector{ Max2D, GroundTraceHalfLength } };
		for (uint8 UintPathMesh = 0; UintPathMesh < PathMeshesNum; ++UintPathMesh)
		{
			InstancesToRemove[UintPathMesh] = PathHISM[UintPathMesh]->GetInstancesOverlappingBox(Box);
		}
	}

	PathPlacement->UnregisterPath(Rect);

	FIntRect const AdjacentRects[4] = {
		FIntRect{ Rect.Min + FIntPoint{ Rect.Width(), 0 }, Rect.Max + FIntPoint{ 1, 0 } },
		FIntRect{ Rect.Min + FIntPoint{ 0, Rect.Height() }, Rect.Max + FIntPoint{ 0, 1 } },
		FIntRect{ Rect.Min - FIntPoint{ 1, 0 }, Rect.Max - FIntPoint{ Rect.Width(), 0 } },
		FIntRect{ Rect.Min - FIntPoint{ 0, 1 }, Rect.Max - FIntPoint{ 0, Rect.Height() } }
	};

	for (FIntRect const & AdjacentRect : AdjacentRects)
	{
		for (int32 X = AdjacentRect.Min.X; X < AdjacentRect.Max.X; ++X)
		{
			for (int32 Y = AdjacentRect.Min.Y; Y < AdjacentRect.Max.Y; ++Y)
			{
				FIntPoint const Coords{ X, Y };
				FAdjacentCellsOccupation AdjacentCellsOccupation;
				GetAdjacentOccupiedCells(Coords, AdjacentCellsOccupation);
				EPathMesh PathMesh;
				FTransform Transform{ ENoInit() };
				GetPathMeshAndTransform(Coords, AdjacentCellsOccupation, PathMesh, Transform);
				uint8 const UintPathMesh = static_cast<uint8>(PathMesh);
				if (LIKELY(UintPathMesh < PathMeshesNum))
				{
					TransformsToAdd[UintPathMesh].Emplace(MoveTemp(Transform));
				}
			}
		}
	}

	for (uint8 UintPathMesh = 0; UintPathMesh < PathMeshesNum; ++UintPathMesh)
	{
		InstancesToRemove[UintPathMesh].Sort(TGreater<int32>());
		RemoveAndAddInstances(PathHISM[UintPathMesh], InstancesToRemove[UintPathMesh], TransformsToAdd[UintPathMesh]);
	}
}

TArray<FIntPoint> const & AUEPathActor::GetPreview() const
{
	return PathPreview;
}

EUEGridLayer AUEPathActor::GetPathRelatedGridLayer() const
{
	check(IsValid(PathPlacement));
	return PathPlacement->GetPathRelatedGridLayer();
}

void AUEPathActor::ClearPreviewData()
{
	for (uint8 UintPathMesh = 0; UintPathMesh < PathMeshesNum; ++UintPathMesh)
	{
		PreviewInstances[UintPathMesh].Empty();
		PrePreviewTransforms[UintPathMesh].Empty();
	}
	PathPreview.Empty();
}

void AUEPathActor::GetAdjacentOccupiedCells(FIntPoint const Coords, FAdjacentCellsOccupation & OutAdjacentCellsOccupation) const
{
	check(IsValid(PathPlacement));
	for (EUEGridDirection const GridDirection : TEnumRange<EUEGridDirection>())
	{
		FIntPoint const AdjacentCoords = FUEGridDirectionUtil::GetAdjacentCoordsUnsafe(Coords, GridDirection);
		if (PathPlacement->IsPathAt(AdjacentCoords))
		{
			OutAdjacentCellsOccupation.NESW[static_cast<uint8>(GridDirection)] = true;
		}
	}
}

void AUEPathActor::GetPathMeshAndInstanceIndex(FIntPoint const Coords, EPathMesh & OutMesh, int32 & OutInstanceIndex) const
{
	FAdjacentCellsOccupation Occupation;
	GetAdjacentOccupiedCells(Coords, Occupation);
	OutMesh = GetPathMesh(Occupation);
	double const Delta = 1e-5;
	FVector2D const CellCenter = UUEGridLibrary::GetGridCellCenter(this, Coords);
	FVector const BoxMin = { CellCenter.X - Delta, CellCenter.Y - Delta, -GroundTraceHalfLength };
	FVector const BoxMax = { CellCenter.X + Delta, CellCenter.Y + Delta, GroundTraceHalfLength };
	TArray<int32> const InstanceIndices = PathHISM[static_cast<uint8>(OutMesh)]->GetInstancesOverlappingBox(FBox{ BoxMin, BoxMax });
	if (InstanceIndices.IsEmpty())
	{
		OutMesh = EPathMesh::NONE;
		OutInstanceIndex = INDEX_NONE;
	}
	else
	{
		OutInstanceIndex = InstanceIndices[0];
	}
}

void AUEPathActor::GetPathMeshAndTransform(FIntPoint const Coords, FAdjacentCellsOccupation const AdjacentCellsOccupation, EPathMesh & OutMesh, FTransform & OutTransform) const
{
	FVector2D const CellCenter = UUEGridLibrary::GetGridCellCenter(this, Coords);
	FVector const Start{ CellCenter, GroundTraceHalfLength };
	FVector const End{ CellCenter, -GroundTraceHalfLength };
	FHitResult HitResult;
	if (UWorld const * const World = GetWorld(); !World || !World->LineTraceSingleByChannel(HitResult, Start, End, GroundTraceChannel))
	{
		OutMesh = EPathMesh::NONE;
		OutTransform = FTransform{};
		return;
	}
	FVector const MeshCenterLocation = HitResult.Location;

	FQuat Rotation;
	GetPathMeshAndRotation(AdjacentCellsOccupation, OutMesh, Rotation);
	OutTransform = FTransform{ Rotation, MeshCenterLocation };
}

AUEPathActor::EPathMesh AUEPathActor::GetPathMesh(FAdjacentCellsOccupation const AdjacentCellsOccupation)
{
	EPathMesh Mesh;
	FQuat Rotation;
	GetPathMeshAndRotation(AdjacentCellsOccupation, Mesh, Rotation);
	return Mesh;
}

void AUEPathActor::GetPathMeshAndRotation(FAdjacentCellsOccupation const AdjacentCellsOccupation, EPathMesh & OutMesh, FQuat & OutRotation)
{
	uint32 OccupationMask = static_cast<bool>(AdjacentCellsOccupation.NESW[0]);
	for (size_t AdjacentCellIndex = 1; AdjacentCellIndex < 4; ++AdjacentCellIndex)
	{
		OccupationMask = (OccupationMask << 1) + static_cast<bool>(AdjacentCellsOccupation.NESW[AdjacentCellIndex]);
	}
	switch (OccupationMask)
	{
		case 0b0000:
		{
			OutMesh = EPathMesh::ZeroConnections;
			OutRotation = FQuat::Identity;
			break;
		}
		case 0b0001:
		case 0b0100:
		case 0b0101:
		{
			OutMesh = EPathMesh::TwoOppositeConnections;
			OutRotation = FQuat::MakeFromRotator(FRotator{ 0, 90, 0 });
			break;
		}
		case 0b0010:
		case 0b1000:
		case 0b1010:
		{
			OutMesh = EPathMesh::TwoOppositeConnections;
			OutRotation = FQuat::Identity;
			break;
		}
		case 0b0011:
		{
			OutMesh = EPathMesh::TwoCloseConnections;
			OutRotation = FQuat::MakeFromRotator(FRotator{ 0, 180, 0 });
			break;
		}
		case 0b0110:
		{
			OutMesh = EPathMesh::TwoCloseConnections;
			OutRotation = FQuat::MakeFromRotator(FRotator{ 0, 90, 0 });
			break;
		}
		case 0b0111:
		{
			OutMesh = EPathMesh::ThreeConnections;
			OutRotation = FQuat::MakeFromRotator(FRotator{ 0, 90, 0 });
			break;
		}
		case 0b1001:
		{
			OutMesh = EPathMesh::TwoCloseConnections;
			OutRotation = FQuat::MakeFromRotator(FRotator{ 0, -90, 0 });
			break;
		}
		case 0b1011:
		{
			OutMesh = EPathMesh::ThreeConnections;
			OutRotation = FQuat::MakeFromRotator(FRotator{ 0, 180, 0 });
			break;
		}
		case 0b1100:
		{
			OutMesh = EPathMesh::TwoCloseConnections;
			OutRotation = FQuat::Identity;
			break;
		}
		case 0b1101:
		{
			OutMesh = EPathMesh::ThreeConnections;
			OutRotation = FQuat::MakeFromRotator(FRotator{ 0, -90, 0 });
			break;
		}
		case 0b1110:
		{
			OutMesh = EPathMesh::ThreeConnections;
			OutRotation = FQuat::Identity;
			break;
		}
		case 0b1111:
		{
			OutMesh = EPathMesh::FourConnections;
			OutRotation = FQuat::Identity;
			break;
		}
		default:
		{
			checkNoEntry();
		}
	}
}