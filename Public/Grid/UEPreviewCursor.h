// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grid/UEGridPlacedActor.h"
#include "UEPreviewCursor.generated.h"

struct FOverlapResult;
class UInputAction;
class UInputMappingContext;

/**
 * AUEPreviewCursor
 */
UCLASS(Abstract)
class UNDEADEMPIRE_API AUEPreviewCursor : public AUEGridPlacedActor
{
	GENERATED_BODY()
	
public:
	explicit AUEPreviewCursor();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;
	virtual void Tick(float const DeltaTime) override;
	virtual void EnableInput(APlayerController * PlayerController) override;
	virtual void DisableInput(APlayerController * PlayerController) override;

	/**
	 * Binds the cursor to player controller's cursor.
	 */
	virtual void BindTo(TObjectPtr<APlayerController> const PlayerController);
	virtual void SetCursorLocation(FVector const & NewPreviewLocation);

	FVector GetPreviewLocation() const;
	FQuat GetPreviewRotation() const;

protected:
	virtual void OnPreviewChanged();
	virtual void SetupPlayerInputComponent(UInputComponent * PlayerInputComponent);
	
	void CancelPreview();
	TArray<FOverlapResult> GetFoliageOverlaps(FIntRect const & Rect) const;
	FVector GetHitLocationUnderMouseCursor() const;
	static void AddInputMappingContext(TObjectPtr<APlayerController> const PlayerController, TObjectPtr<UInputMappingContext> const InputMappinngContext, int32 const Priority = 0);
	static void RemoveInputMappingContext(TObjectPtr<APlayerController> const PlayerController, TObjectPtr<UInputMappingContext> const InputMappinngContext);
	static void HideAndShowOverlappedFoliage(TArray<FOverlapResult> const & OldOverlaps, TArray<FOverlapResult> const & NewOverlaps);
	static void RemoveOverlappedFoliage(TArray<FOverlapResult> const & Overlaps);

	UPROPERTY(BlueprintReadOnly)
	FVector PreviousPreviewLocation;

	/**
	 * Player controller to which cursor is bound.
	 */
	UPROPERTY()
	TObjectPtr<APlayerController> BoundPlayerController;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> PreviewCursorInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> CancelPreviewInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	float FoliageOverlapHalfZExtent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel;

	/**
	 * Channel used to find overlapped foliage.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	TEnumAsByte<ECollisionChannel> FoliageOverlapChannel;
};
