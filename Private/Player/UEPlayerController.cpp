// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/UEPlayerController.h"
#include "Building/UEBuildingActor.h"
#include "Building/UEBuildingPreviewCursor.h"
#include "Common/UELog.h"
#include "Grid/UEDemolitionPreviewCursor.h"
#include "Path/UEPathActor.h"
#include "Path/UEPathPreviewCursor.h"
#include "Player/UEPlayerState.h"

namespace
{
    template <typename PreviewCursorType, typename ActorClassType>
    void FinishDeferConstruction(TObjectPtr<PreviewCursorType> const PreviewCursor, TObjectPtr<AUEPlayerController> const PlayerController, ActorClassType const ActorClass)
    {
        if (PreviewCursor)
        {
            PreviewCursor->BindTo(PlayerController);
            PreviewCursor->SetupFor(ActorClass);
            PreviewCursor->FinishSpawning(FTransform{});
            UE_LOGFMT(LogUE, Log, "Preview cursor for \"{0}\" spawned.", ActorClass->GetName());
        }
        else
        {
            UE_LOGFMT(LogUE, Warning, "Defer construction of \"{0}\" preview cursor failed.", ActorClass->GetName());
        }
    }

    template <>
    void FinishDeferConstruction(TObjectPtr<AUEDemolitionPreviewCursor> const PreviewCursor, TObjectPtr<AUEPlayerController> const PlayerController, std::nullptr_t const ActorClass)
    {
        if (PreviewCursor)
        {
            PreviewCursor->BindTo(PlayerController);
            PreviewCursor->FinishSpawning(FTransform{});
            UE_LOGFMT(LogUE, Log, "Demolition preview cursor spawned.");
        }
        else
        {
            UE_LOGFMT(LogUE, Warning, "Defer construction of demolition preview cursor failed.");
        }
    }

    template <typename PreviewCursorType, typename ActorClassType>
    TObjectPtr<PreviewCursorType> SpawnPreviewCursor(TObjectPtr<AUEPlayerController> const PlayerController, TSubclassOf<PreviewCursorType> const PreviewCursorClass, ActorClassType const ActorClass)
    {
        if (!IsValid(PlayerController) || !IsValid(PreviewCursorClass))
        {
            return nullptr;
        }
        TObjectPtr<UWorld> World = PlayerController->GetWorld();
        if (!IsValid(World))
        {
            return nullptr;
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        SpawnParams.bDeferConstruction = true;

        TObjectPtr<PreviewCursorType> PreviewCursor = World->SpawnActor<PreviewCursorType>(PreviewCursorClass, SpawnParams);
        FinishDeferConstruction(PreviewCursor, PlayerController, ActorClass);
        return PreviewCursor;
    }
} // namespace

AUEPlayerController::AUEPlayerController()
    : BuildingPreviewCursorClass(AUEBuildingPreviewCursor::StaticClass())
    , PathPreviewCursorClass(AUEPathPreviewCursor::StaticClass())
    , DemolitionPreviewCursorClass(AUEDemolitionPreviewCursor::StaticClass())
    , PreviewCursor(nullptr)
{
}

TObjectPtr<AUEPlayerState> AUEPlayerController::GetUEPlayerState() const
{
	return Cast<AUEPlayerState>(PlayerState);
}

void AUEPlayerController::BeginPlacement(TSubclassOf<AUEBuildingActor> const BuildingActorClass)
{
    if (!BuildingActorClass)
    {
        return;
    }

    CancelPreview();

    PreviewCursor = SpawnPreviewCursor(this, BuildingPreviewCursorClass, BuildingActorClass);
}

void AUEPlayerController::BeginPlacement(TSubclassOf<AUEPathActor> const PathActorClass)
{
    if (!PathActorClass)
    {
        return;
    }

    CancelPreview();

    PreviewCursor = SpawnPreviewCursor(this, PathPreviewCursorClass, PathActorClass);
}

void AUEPlayerController::BeginDemolition()
{
    CancelPreview();

    PreviewCursor = SpawnPreviewCursor(this, DemolitionPreviewCursorClass, nullptr);
}

void AUEPlayerController::CancelPreview()
{
    if (IsValid(PreviewCursor))
    {
        PreviewCursor->Destroy();
    }
}

void AUEPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    APlayerController::bShowMouseCursor = true;
    APlayerController::bEnableClickEvents = true;
    APlayerController::bEnableMouseOverEvents = true;
}