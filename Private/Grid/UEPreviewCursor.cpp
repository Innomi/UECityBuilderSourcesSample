// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEPreviewCursor.h"
#include "Common/UEOverlapResultUtilities.h"
#include "Components/InputComponent.h"
#include "Engine/OverlapResult.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Foliage/UEFoliageInstancedStaticMeshComponent.h"
#include "Grid/UEGridLibrary.h"
#include "Grid/UEGridPlacementComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace
{
	struct FIsFoliageOverlap
	{
		bool operator () (FOverlapResult const & OverlapResult)
		{
			UPrimitiveComponent const * const Component = OverlapResult.GetComponent();
			return IsValid(Component) && Component->IsA(UUEFoliageInstancedStaticMeshComponent::StaticClass());
		}
	};
} // namespace

AUEPreviewCursor::AUEPreviewCursor()
{
	PrimaryActorTick.bCanEverTick = true;
	BoundPlayerController = nullptr;
	PreviewCursorInputMappingContext = nullptr;
	CancelPreviewInputAction = nullptr;
	RootComponent->SetMobility(EComponentMobility::Type::Movable);
	GridPlacement->SetMobility(EComponentMobility::Type::Movable);
	GridPlacement->SetShouldBeRegisteredOnGrid(false);
	InputComponent = CreateDefaultSubobject<UEnhancedInputComponent>("EnhancedInput");
	FoliageOverlapHalfZExtent = 9e4f;
	GroundTraceChannel = ECollisionChannel::ECC_WorldStatic;
	FoliageOverlapChannel = ECollisionChannel::ECC_GameTraceChannel1;
}

void AUEPreviewCursor::BeginPlay()
{
	Super::BeginPlay();

	SetupPlayerInputComponent(InputComponent);
	if (IsValid(BoundPlayerController))
	{
		EnableInput(BoundPlayerController);
		SetCursorLocation(GetHitLocationUnderMouseCursor());
		PreviousPreviewLocation = GetPreviewLocation();
	}
}

void AUEPreviewCursor::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	if (IsValid(BoundPlayerController))
	{
		DisableInput(BoundPlayerController);
		BoundPlayerController = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AUEPreviewCursor::Tick(float const DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsValid(BoundPlayerController))
	{
		SetCursorLocation(GetHitLocationUnderMouseCursor());
		FVector const CurrentPreviewLocation = GetPreviewLocation();
		if (!CurrentPreviewLocation.Equals(PreviousPreviewLocation))
		{
			OnPreviewChanged();
			PreviousPreviewLocation = CurrentPreviewLocation;
		}
	}
}

void AUEPreviewCursor::EnableInput(APlayerController * PlayerController)
{
	Super::EnableInput(PlayerController);

	AddInputMappingContext(PlayerController, PreviewCursorInputMappingContext);
}

void AUEPreviewCursor::DisableInput(APlayerController * PlayerController)
{
	RemoveInputMappingContext(PlayerController, PreviewCursorInputMappingContext);

	Super::DisableInput(PlayerController);
}

void AUEPreviewCursor::BindTo(TObjectPtr<APlayerController> const PlayerController)
{
	if (IsValid(PlayerController))
	{
		if (HasActorBegunPlay())
		{
			if (IsValid(BoundPlayerController))
			{
				DisableInput(BoundPlayerController);
			}
			EnableInput(PlayerController);
		}
		BoundPlayerController = PlayerController;
	}
}

void AUEPreviewCursor::SetCursorLocation(FVector const & NewPreviewLocation)
{
	FVector const GridCenterOffset = GetPreviewLocation() - GetActorLocation();
	FVector const NotSnappedActorLocation = NewPreviewLocation - GridCenterOffset;
	SetActorLocation(GridPlacement->GridSnapLocation(NotSnappedActorLocation));
}

FVector AUEPreviewCursor::GetPreviewLocation() const
{
	check(IsValid(GridPlacement));
	return GridPlacement->GetSocketLocation(UUEGridPlacementComponent::GridCenterSocketName);
}

FQuat AUEPreviewCursor::GetPreviewRotation() const
{
	check(IsValid(GridPlacement));
	return GridPlacement->GetSocketQuaternion(UUEGridPlacementComponent::GridCenterSocketName);
}

void AUEPreviewCursor::OnPreviewChanged()
{
}

void AUEPreviewCursor::SetupPlayerInputComponent(UInputComponent * PlayerInputComponent)
{
	TObjectPtr<UEnhancedInputComponent> EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	check(EnhancedInput && CancelPreviewInputAction);

	EnhancedInput->BindAction(CancelPreviewInputAction, ETriggerEvent::Triggered, this, &AUEPreviewCursor::CancelPreview);
}

void AUEPreviewCursor::CancelPreview()
{
	Destroy();
}

TArray<FOverlapResult> AUEPreviewCursor::GetFoliageOverlaps(FIntRect const & Rect) const
{
	if (Rect.IsEmpty() || !UUEGridLibrary::IsInSingleGridComponent(this, Rect))
	{
		return TArray<FOverlapResult>{};
	}

	double const GridCellSize = UUEGridLibrary::GetGridCellSize(this);
	FVector2D const Min{ Rect.Min.X * GridCellSize, Rect.Min.Y * GridCellSize };
	FVector2D const Max{ Rect.Max.X * GridCellSize, Rect.Max.Y * GridCellSize };
	FVector2D const BoxHalfExtent2D = (Max - Min) * 0.5;
	FVector2D const BoxPos2D = Min + BoxHalfExtent2D;
	FVector const BoxHalfExtent{ BoxHalfExtent2D, FoliageOverlapHalfZExtent };
	FVector const BoxPos{ BoxPos2D, 0. };
	TObjectPtr<UWorld> const World = GetWorld();
	check(IsValid(World));
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, BoxPos, FQuat::Identity, FoliageOverlapChannel, FCollisionShape::MakeBox(BoxHalfExtent));
	return Overlaps;
}

FVector AUEPreviewCursor::GetHitLocationUnderMouseCursor() const
{
	if (!IsValid(BoundPlayerController))
	{
		return FVector::ZeroVector;
	}

	FHitResult HitResult;
	BoundPlayerController->GetHitResultUnderCursor(GroundTraceChannel, false, HitResult);
	return HitResult.ImpactPoint;
}

void AUEPreviewCursor::AddInputMappingContext(TObjectPtr<APlayerController> const PlayerController, TObjectPtr<UInputMappingContext> const InputMappingContext, int32 const Priority)
{
	if (!InputMappingContext || !IsValid(PlayerController))
	{
		return;
	}
	if (TObjectPtr<ULocalPlayer> LocalPlayer = PlayerController->GetLocalPlayer())
	{
		if (TObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSystem->AddMappingContext(InputMappingContext, Priority);
		}
	}
}

void AUEPreviewCursor::RemoveInputMappingContext(TObjectPtr<APlayerController> const PlayerController, TObjectPtr<UInputMappingContext> const InputMappingContext)
{
	if (!InputMappingContext || !IsValid(PlayerController))
	{
		return;
	}
	if (TObjectPtr<ULocalPlayer> LocalPlayer = PlayerController->GetLocalPlayer())
	{
		if (TObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSystem->RemoveMappingContext(InputMappingContext);
		}
	}
}

void AUEPreviewCursor::HideAndShowOverlappedFoliage(TArray<FOverlapResult> const & OldOverlaps, TArray<FOverlapResult> const & NewOverlaps)
{
	TArray<FOverlapResult> OldFoliageOverlaps = OldOverlaps.FilterByPredicate(FIsFoliageOverlap());
	TArray<FOverlapResult> NewFoliageOverlaps = NewOverlaps.FilterByPredicate(FIsFoliageOverlap());
	for (int32 OldOverlapIndex = 0; OldOverlapIndex < OldFoliageOverlaps.Num() && NewFoliageOverlaps.Num() > 0; ++OldOverlapIndex)
	{
		FOverlapResult const & SearchItem = OldFoliageOverlaps[OldOverlapIndex];
		int32 const NewOverlapIndex = NewFoliageOverlaps.IndexOfByPredicate(FOverlapResultCompare(SearchItem));
		if (NewOverlapIndex != INDEX_NONE)
		{
			NewFoliageOverlaps.RemoveAtSwap(NewOverlapIndex, 1, EAllowShrinking::No);
			OldFoliageOverlaps.RemoveAtSwap(OldOverlapIndex, 1, EAllowShrinking::No);
			--OldOverlapIndex;
		}
	}
	TArray<UUEFoliageInstancedStaticMeshComponent *> OverlappedFoliageComponents;
	auto SetInstancesVisibility = [&OverlappedFoliageComponents](TArray<FOverlapResult> const & FoliageOverlaps, bool const bIsVisible)
		{
			for (FOverlapResult const & FoliageOverlap : FoliageOverlaps)
			{
				UUEFoliageInstancedStaticMeshComponent * const FoliageComponent = CastChecked<UUEFoliageInstancedStaticMeshComponent>(FoliageOverlap.GetComponent());
				OverlappedFoliageComponents.AddUnique(FoliageComponent);
				int32 const InstanceIndex = FoliageOverlap.ItemIndex;
				FoliageComponent->SetInstanceVisibility(InstanceIndex, bIsVisible);
			}
		};
	{
		bool const bIsVisible = true;
		SetInstancesVisibility(OldFoliageOverlaps, bIsVisible);
	}
	{
		bool const bIsVisible = false;
		SetInstancesVisibility(NewFoliageOverlaps, bIsVisible);
	}
	for (UUEFoliageInstancedStaticMeshComponent * FoliageComponent : OverlappedFoliageComponents)
	{
		FoliageComponent->MarkRenderStateDirty();
	}
}

void AUEPreviewCursor::RemoveOverlappedFoliage(TArray<FOverlapResult> const & Overlaps)
{
	TArray<FOverlapResult> FoliageOverlaps = Overlaps.FilterByPredicate(FIsFoliageOverlap());
	TMap<UUEFoliageInstancedStaticMeshComponent *, TArray<int32>> OverlappedFoliage;
	for (FOverlapResult const & FoliageOverlap : FoliageOverlaps)
	{
		UPrimitiveComponent * const OverlappedComponent = FoliageOverlap.GetComponent();
		UUEFoliageInstancedStaticMeshComponent * const FoliageComponent = CastChecked<UUEFoliageInstancedStaticMeshComponent>(OverlappedComponent);
		int32 const InstanceIndex = FoliageOverlap.ItemIndex;
		OverlappedFoliage.FindOrAdd(FoliageComponent).Emplace(InstanceIndex);
	}
	for (TPair<UUEFoliageInstancedStaticMeshComponent *, TArray<int32>> OverlappedFoliageEntry : OverlappedFoliage)
	{
		OverlappedFoliageEntry.Key->RemoveInstances(OverlappedFoliageEntry.Value);
	}
}