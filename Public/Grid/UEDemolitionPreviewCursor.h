// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grid/UEPreviewCursor.h"
#include "UEDemolitionPreviewCursor.generated.h"

class UInputMappingContext;
class UInputAction;
struct FOverlapResult;

/**
 * AUEDemolitionPreviewCursor
 */
UCLASS()
class UNDEADEMPIRE_API AUEDemolitionPreviewCursor : public AUEPreviewCursor
{
	GENERATED_BODY()

public:
	explicit AUEDemolitionPreviewCursor();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;
	virtual void EnableInput(APlayerController * PlayerController) override;
	virtual void DisableInput(APlayerController * PlayerController) override;

protected:
	enum class EDemolitionType : uint8
	{
		Foliage,
		Building,
		Road,
		NONE
	};

	virtual void OnPreviewChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent * PlayerInputComponent) override;

	EDemolitionType GetDemolitionType(FIntPoint const Coords) const;
	void UpdateDecalTransform(FIntRect const & Rect);
	void UpdateDemolitionPreview(FIntRect && NewDemolitionPreview, EDemolitionType const NewDemolitionType);
	void StartSelecting();
	void FinishSelecting();
	void CancelSelecting();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	TObjectPtr<UDecalComponent> SelectionDecal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> DemolitionPreviewCursorInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> StartSelectingInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> FinishSelectingInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> CancelSelectingInputAction;

	FIntRect PreviousDemolitionPreviev;
	FIntPoint SelectionStartLocationOnGrid;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	float SelectionDecalHalfZExtent;
	
	EDemolitionType PreviousDemolitionType;
	uint8 bIsSelecting : 1;
};
