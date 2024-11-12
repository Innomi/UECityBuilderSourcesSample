// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Common/UEActor.h"
#include "UEPathActor.generated.h"

enum class EUEGridLayer : uint8;
class UHierarchicalInstancedStaticMeshComponent;
class UUEPathPlacementComponent;

/**
 * AUEPathActor
 */
UCLASS()
class UNDEADEMPIRE_API AUEPathActor : public AUEActor
{
	GENERATED_BODY()

public:
	AUEPathActor();

	virtual void PostRegisterAllComponents() override;
	virtual void UnregisterAllComponents(bool const bForReregister = false) override;

	/*
	 * Creates preview. If there was one it will be replaced.
	 * @param Path - grid coords to place preview on.
	 */
	void CreatePreview(TArray<FIntPoint> const & Path);
	void CancelPreview();
	void CommitPreview();
	void RemovePath(FIntRect const & Rect);
	TArray<FIntPoint> const & GetPreview() const;
	EUEGridLayer GetPathRelatedGridLayer() const;

protected:
	enum class EPathMesh : uint8
	{
		ZeroConnections,
		TwoOppositeConnections,
		TwoCloseConnections,
		ThreeConnections,
		FourConnections,
		PATH_MESHES_NUM,
		NONE
	};

	struct FAdjacentCellsOccupation
	{
		uint8 NESW[4]{};
	};

	static constexpr uint8 PathMeshesNum = static_cast<uint8>(EPathMesh::PATH_MESHES_NUM);

	void ClearPreviewData();
	void GetAdjacentOccupiedCells(FIntPoint const Coords, FAdjacentCellsOccupation & OutAdjacentCellsOccupation) const;
	void GetPathMeshAndInstanceIndex(FIntPoint const Coords, EPathMesh & OutMesh, int32 & OutInstanceIndex) const;
	void GetPathMeshAndTransform(FIntPoint const Coords, FAdjacentCellsOccupation const AdjacentCellsOccupation, EPathMesh & OutMesh, FTransform & OutTransform) const;

	static EPathMesh GetPathMesh(FAdjacentCellsOccupation const AdjacentCellsOccupation);
	static void GetPathMeshAndRotation(FAdjacentCellsOccupation const AdjacentCellsOccupation, EPathMesh & OutMesh, FQuat & OutRotation);

	TArray<FIntPoint> PathPreview;
	TArray<int32> PreviewInstances[PathMeshesNum];
	TArray<FTransform> PrePreviewTransforms[PathMeshesNum];

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UUEPathPlacementComponent> PathPlacement;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ZeroConnectionsPathHISM;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TwoOppositeConnectionsPathHISM;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TwoCloseConnectionsPathHISM;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ThreeConnectionsPathHISM;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FourConnectionsPathHISM;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> PathHISM[PathMeshesNum];

	// Length of tracepath from 0. along positive and negative Z directions. Needed to place meshes on the ground.
	UPROPERTY(EditDefaultsOnly)
	float GroundTraceHalfLength;

	// The channel used to place meshes.
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ECollisionChannel> GroundTraceChannel;
};
