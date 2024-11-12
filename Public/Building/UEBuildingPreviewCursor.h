// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grid/UEPreviewCursor.h"
#include "UEBuildingPreviewCursor.generated.h"

class AUEBuildingActor;
class UInputMappingContext;
class UInputAction;
struct FOverlapResult;

/**
 * AUEBuildingPreviewCursor
 */
UCLASS()
class UNDEADEMPIRE_API AUEBuildingPreviewCursor : public AUEPreviewCursor
{
	GENERATED_BODY()

public:
	explicit AUEBuildingPreviewCursor();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;
	virtual void EnableInput(APlayerController * PlayerController) override;
	virtual void DisableInput(APlayerController * PlayerController) override;

	/**
	 * Initializes this preview cursor with the specified class.
	 */
	void SetupFor(TSubclassOf<AUEBuildingActor> const BuildingActorClass);
	bool IsCursorSetup() const;

protected:
	virtual void OnPreviewChanged() override;

	TArray<FOverlapResult> GetFoliageOverlaps(FVector const & Location, FQuat const & Rotation) const;
	virtual void SetupPlayerInputComponent(UInputComponent * PlayerInputComponent) override;
	void TryToPlaceBuilding();
	void RotateAndGridSnap(double const DeltaYaw);
	void RotateClockwise();
	void RotateCounterclockwise();

	UPROPERTY(BlueprintReadOnly)
	FQuat PreviousPreviewRotation;

	/**
	 * Class of building to preview.
	 */
	UPROPERTY()
	TSubclassOf<AUEBuildingActor> PreviewBuildingClass;

	/**
	 * Mesh of building to preview.
	 */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> BuildingPreviewCursorInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> TryToPlaceBuildingInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> RotateClockwiseInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> RotateCounterclockwiseInputAction;
};
