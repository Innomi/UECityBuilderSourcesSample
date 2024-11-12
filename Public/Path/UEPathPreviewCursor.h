// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grid/UEPreviewCursor.h"
#include "UEPathPreviewCursor.generated.h"

struct FOverlapResult;
class AUEPathActor;
class UInputMappingContext;

/**
 * AUEPathPreviewCursor
 */
UCLASS()
class UNDEADEMPIRE_API AUEPathPreviewCursor : public AUEPreviewCursor
{
	GENERATED_BODY()

public:
	explicit AUEPathPreviewCursor();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;
	virtual void EnableInput(APlayerController * PlayerController) override;
	virtual void DisableInput(APlayerController * PlayerController) override;

	/**
	 * Initializes this preview cursor with the specified path actor.
	 */
	void SetupFor(TSubclassOf<AUEPathActor> const PathActorClass);

protected:
	virtual void OnPreviewChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent * PlayerInputComponent) override;

	TArray<FOverlapResult> GetFoliageOverlaps(TArray<FIntPoint> const & Path) const;
	TArray<FIntPoint> GetPath(FIntPoint const PathStartLocationOnGrid, FIntPoint const PathEndLocationOnGrid) const;
	void UpdatePathPreview(TArray<FIntPoint> && NewPathPreview);
	void StartPathPlacing();
	void FinishPathPlacing();
	void CancelPathPlacing();
	void ChangePriorityAxis();

	TArray<FIntPoint> PreviousPathPreview;

	/**
	 * Path actor for preview.
	 */
	UPROPERTY()
	TObjectPtr<AUEPathActor> PathActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> PathPreviewCursorInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> StartPathPlacingInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> FinishPathPlacingInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> CancelPathPlacingInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> ChangePriorityAxisInputAction;

	FIntPoint PathStartLocationOnGrid;
	uint8 bIsGoingAlongXFirst : 1;
	uint8 bIsPlacingPath : 1;
};