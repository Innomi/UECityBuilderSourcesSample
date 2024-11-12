// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/UETopDownCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Math/UnrealMathUtility.h"

AUETopDownCameraPawn::AUETopDownCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	ForwardRightMovementInput = FVector2D::Zero();
	ZoomInInput = 0.f;
	ClockwiseRotationInput = 0.f;
	PreviousHeight = 0.f;
	MovementMultiplier = 5000.f;
	RotationMultiplier = 120.f;
	ZoomMultiplier = 200.f;
	ZoomLagSpeed = 4.f;
	MinArmAngle = 30;
	MaxArmAngle = 60;
	ZoomThresholdToAdjustAngle = 2500.f;
	MinSpringArm = 500.f;
	MaxSpringArm = 4000.f;
	TargetZoom = (MaxSpringArm + MinSpringArm) / 2.f;
	GroundTraceHalfLength = 9e4;
	HeightThreshold = 40.f;
	CameraEdgeScrollThreshold = 0.02f;
	TraceChannel = ECollisionChannel::ECC_WorldStatic;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	TopDownCameraInputMappingContext = nullptr;
	MoveInputAction = nullptr;
	ZoomInputAction = nullptr;
	RotateInputAction = nullptr;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("RootComponent"));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = TargetZoom;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 4.f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 4.f;
	SpringArm->bDoCollisionTest = false;
	AdjustArmAngle();

	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm);
}

void AUETopDownCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	PreviousHeight = GetActorLocation().Z;
	TargetZoom = (MaxSpringArm + MinSpringArm) / 2.f;
	AdjustArmAngle();
}

void AUETopDownCameraPawn::Tick(float const DeltaTime)
{
	Super::Tick(DeltaTime);

	ProcessEdgeScrolling();
	ApplyMovement(DeltaTime);
	ApplyRotation(DeltaTime);
	ApplyZoom(DeltaTime);
}

void AUETopDownCameraPawn::SetupPlayerInputComponent(UInputComponent * PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	TObjectPtr<UEnhancedInputComponent> EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	check(EnhancedInput && MoveInputAction && ZoomInputAction && RotateInputAction);

	EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &AUETopDownCameraPawn::MoveForwardRight);
	EnhancedInput->BindAction(ZoomInputAction, ETriggerEvent::Triggered, this, &AUETopDownCameraPawn::ZoomIn);
	EnhancedInput->BindAction(RotateInputAction, ETriggerEvent::Triggered, this, &AUETopDownCameraPawn::RotateClockwise);
}

void AUETopDownCameraPawn::NotifyControllerChanged()
{
	if (TObjectPtr<APlayerController> PreviousPlayerController = Cast<APlayerController>(PreviousController))
	{
		if (TObjectPtr<ULocalPlayer> LocalPlayer = PreviousPlayerController->GetLocalPlayer())
		{
			if (TObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSystem->RemoveMappingContext(TopDownCameraInputMappingContext);
			}
		}
	}

	if (TObjectPtr<APlayerController> PlayerController = Cast<APlayerController>(Controller))
	{
		if (TObjectPtr<ULocalPlayer> LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (TObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSystem->AddMappingContext(TopDownCameraInputMappingContext, 0);
			}
		}
	}

	Super::NotifyControllerChanged();
}

void AUETopDownCameraPawn::ProcessEdgeScrolling()
{
	APlayerController const * PlayerController = GetController<APlayerController const>();
	UGameViewportClient const * Viewport = GetWorld()->GetGameViewport();
	FVector2D MousePosition;
	if (!IsValid(PlayerController) || !IsValid(Viewport) || !PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y))
	{
		return;
	}
	FVector2D ViewportSize;
	Viewport->GetViewportSize(ViewportSize);
	
	double const ScrollBorderLeft = ViewportSize.X * CameraEdgeScrollThreshold;
	double const ScrollBorderTop = ViewportSize.Y * CameraEdgeScrollThreshold;
	double const ScrollBorderRight = ViewportSize.X - ScrollBorderLeft;
	double const ScrollBorderBottom = ViewportSize.Y - ScrollBorderTop;

	ForwardRightMovementInput.X -= FMath::Clamp(FMath::GetRangePct(ScrollBorderBottom, ViewportSize.Y, MousePosition.Y), 0., 1.);
	ForwardRightMovementInput.X += FMath::Clamp(FMath::GetRangePct(ScrollBorderTop, 0., MousePosition.Y), 0., 1.);
	ForwardRightMovementInput.Y -= FMath::Clamp(FMath::GetRangePct(ScrollBorderLeft, 0., MousePosition.X), 0., 1.);
	ForwardRightMovementInput.Y += FMath::Clamp(FMath::GetRangePct(ScrollBorderRight, ViewportSize.X, MousePosition.X), 0., 1.);

	ForwardRightMovementInput = ForwardRightMovementInput.ClampAxes(-1., 1.);
}

// TODO: add world boundaries for movement
void AUETopDownCameraPawn::ApplyMovement(float const DeltaTime)
{
	if (!ForwardRightMovementInput.IsNearlyZero())
	{
		FVector NewLocation = GetActorLocation();
		NewLocation += GetActorForwardVector() * ForwardRightMovementInput.X * MovementMultiplier * DeltaTime;
		NewLocation += GetActorRightVector() * ForwardRightMovementInput.Y * MovementMultiplier * DeltaTime;
		AdjustHeight(NewLocation);
		SetActorLocation(NewLocation);
		ForwardRightMovementInput = FVector2D::Zero();
	}
}

void AUETopDownCameraPawn::ApplyZoom(float const DeltaTime)
{
	if (!IsValid(SpringArm))
	{
		return;
	}

	TargetZoom = FMath::Clamp<float>(TargetZoom + ZoomInInput * ZoomMultiplier, MinSpringArm, MaxSpringArm);
	float const CurrentArmLength = SpringArm->TargetArmLength;
	if (!FMath::IsNearlyEqual(TargetZoom, CurrentArmLength))
	{
		SpringArm->TargetArmLength = FMath::FInterpTo(CurrentArmLength, TargetZoom, DeltaTime, ZoomLagSpeed);
	}
	if (!FMath::IsNearlyZero(ZoomInInput))
	{
		AdjustArmAngle();
		ZoomInInput = 0.f;
	}
}

void AUETopDownCameraPawn::ApplyRotation(float const DeltaTime)
{
	if (!FMath::IsNearlyZero(ClockwiseRotationInput))
	{
		FRotator NewRotation = GetActorRotation();
		NewRotation.Yaw += ClockwiseRotationInput * RotationMultiplier * DeltaTime;
		SetActorRotation(NewRotation);
		ClockwiseRotationInput = 0.f;
	}
}

void AUETopDownCameraPawn::AdjustHeight(FVector & OutLocation)
{
	TObjectPtr<UWorld> const World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	FHitResult HitResult;
	FVector Start = OutLocation;
	FVector End = OutLocation;
	Start.Z += GroundTraceHalfLength;
	End.Z -= GroundTraceHalfLength;
	World->LineTraceSingleByChannel(HitResult, Start, End, TraceChannel);

	if (FMath::Abs(PreviousHeight - HitResult.Location.Z) >= HeightThreshold)
	{
		PreviousHeight = HitResult.Location.Z;
		OutLocation = HitResult.Location;
	}
}

void AUETopDownCameraPawn::AdjustArmAngle()
{
	if (!IsValid(SpringArm))
	{
		return;
	}

	if (TargetZoom > ZoomThresholdToAdjustAngle)
	{
		SpringArm->SetRelativeRotation(FRotator{ -MaxArmAngle, 0.f, 0.f });
		return;
	}

	float const ArmLengthRatio = FMath::Clamp((TargetZoom - MinSpringArm) / (ZoomThresholdToAdjustAngle - MinSpringArm), -1.f, 1.f);
	float const NewAngle = ArmLengthRatio * (MaxArmAngle - MinArmAngle) + MinArmAngle;
	SpringArm->SetRelativeRotation(FRotator{ -NewAngle, 0.f, 0.f });
}

void AUETopDownCameraPawn::MoveForwardRight(FInputActionValue const & ActionValue)
{
	ForwardRightMovementInput = ActionValue.Get<FVector2D>().ClampAxes(-1., 1.);
}

void AUETopDownCameraPawn::ZoomIn(FInputActionValue const & ActionValue)
{
	ZoomInInput = FMath::Clamp(ActionValue.Get<float>(), -1.f, 1.f);
}

void AUETopDownCameraPawn::RotateClockwise(FInputActionValue const & ActionValue)
{
	ClockwiseRotationInput = FMath::Clamp(ActionValue.Get<float>(), -1.f, 1.f);
}