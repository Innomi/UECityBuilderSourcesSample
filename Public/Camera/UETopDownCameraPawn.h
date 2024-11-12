// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UETopDownCameraPawn.generated.h"

struct FInputActionValue;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;

UCLASS()
class UNDEADEMPIRE_API AUETopDownCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AUETopDownCameraPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float const DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent * PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> TopDownCameraInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> MoveInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> ZoomInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> RotateInputAction;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UCameraComponent> Camera;

private:
	// Input variables
	FVector2D ForwardRightMovementInput;
	float ZoomInInput;
	float ClockwiseRotationInput;
	
	// To check whether the height threshold has been reached
	float PreviousHeight;

	// To make zoom smooth
	float TargetZoom;

protected:
	// By how much the AxisValue should be multiplied
	UPROPERTY(EditAnywhere)
	float MovementMultiplier;

	// By how much the AxisValue should be multiplied
	UPROPERTY(EditAnywhere)
	float RotationMultiplier;

	// By how much the AxisValue should be multiplied
	UPROPERTY(EditAnywhere)
	float ZoomMultiplier;

	UPROPERTY(EditAnywhere)
	float ZoomLagSpeed;

	UPROPERTY(EditAnywhere)
	float ZoomThresholdToAdjustAngle;

	UPROPERTY(EditAnywhere)
	float MinArmAngle;

	UPROPERTY(EditAnywhere)
	float MaxArmAngle;

	// Minimum length of the spring arm
	UPROPERTY(EditAnywhere)
	float MinSpringArm;

	// Maximum length of the spring arm
	UPROPERTY(EditAnywhere)
	float MaxSpringArm;

	// Length of tracepath from actor location along positive and negative Z directions
	UPROPERTY(EditDefaultsOnly)
	float GroundTraceHalfLength;

	// The threshold required to adjust the camera
	UPROPERTY(EditDefaultsOnly)
	float HeightThreshold;

	// Portion of viewport to trigger camera scrolling
	UPROPERTY(EditDefaultsOnly)
	float CameraEdgeScrollThreshold;

	// The channel used for the height adjustment
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ECollisionChannel> TraceChannel;

private:
	void ProcessEdgeScrolling();
	void ApplyMovement(float const DeltaTime);
	void ApplyZoom(float const DeltaTime);
	void ApplyRotation(float const DeltaTime);
	void AdjustHeight(FVector & OutLocation);
	void AdjustArmAngle();

	// Input actions
	void MoveForwardRight(FInputActionValue const & ActionValue);
	void ZoomIn(FInputActionValue const & ActionValue);
	void RotateClockwise(FInputActionValue const & ActionValue);
};
