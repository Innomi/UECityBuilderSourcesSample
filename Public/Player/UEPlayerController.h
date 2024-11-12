// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UEPlayerController.generated.h"

class AUEBuildingActor;
class AUEBuildingPreviewCursor;
class AUEDemolitionPreviewCursor;
class AUEPathActor;
class AUEPathPreviewCursor;
class AUEPlayerState;
class AUEPreviewCursor;

/**
 * AUEPlayerController
 * The base player controller class used by this project.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class UNDEADEMPIRE_API AUEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AUEPlayerController();

	/**
	 * Gets the replicated state of this player.
	 */
	TObjectPtr<AUEPlayerState> GetUEPlayerState() const;

	/**
	 * Begin finding a suitable location for the specified class.
	 */
	void BeginPlacement(TSubclassOf<AUEBuildingActor> const BuildingActorClass);
	void BeginPlacement(TSubclassOf<AUEPathActor> const PathActorClass);
	void BeginDemolition();

	void CancelPreview();

protected:
	virtual void SetupInputComponent() override;

	/**
	 * Preview cursor to use for building placement.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "UE")
	TSubclassOf<AUEBuildingPreviewCursor> BuildingPreviewCursorClass;

	/**
	 * Preview cursor to use for path placement.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "UE")
	TSubclassOf<AUEPathPreviewCursor> PathPreviewCursorClass;

	/**
	 * Preview cursor to use for demolition.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "UE")
	TSubclassOf<AUEDemolitionPreviewCursor> DemolitionPreviewCursorClass;

	/**
	 * Current cursor.
	 */
	UPROPERTY()
	TObjectPtr<AUEPreviewCursor> PreviewCursor;
};