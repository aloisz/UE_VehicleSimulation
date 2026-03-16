// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleAerodynamicsComponent.generated.h"


// forward declaration
class UVehicleWheelSystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable )
class VEHICLESIMULATION_API UVehicleAerodynamicsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVehicleAerodynamicsComponent();

	void UpdateAerodynamics(float DeltaTime, UPrimitiveComponent* VehicleRoot, const USceneComponent* CenterOfMass) const;
	void UpdateStabilization(float DeltaTime, UPrimitiveComponent* VehicleRoot);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
	float DragCoefficient = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
	float DownForce = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
	float FrontalArea = 2.5f; // m^2

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stabilization")
	float TractionControlStrength = 0.3f;

	// === Roll ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stabilization|Roll")
	float RollAngleMultiplier = 2000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stabilization|Roll")
	float RollVelocityMultiplier = 500.0f;

	// === Pitch ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stabilization|Pitch")
	float PitchAngleMultiplier = 1500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stabilization|Pitch")
	float PitchVelocityMultiplier = 300.0f;

	// === Yaw ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stabilization|Yaw")
	float YawVelocityMultiplier = 200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stabilization|Yaw")
	float YawSpeedDivider = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stabilization|Yaw")
	float EnableYawDampingAtSpeed = 80.0f; // km/h

	// Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bIsInDebug = false;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	// === Stabilization ===
	FVector CalculateAntiRoll(const FVector& AngularVelocity, const FRotator& VehicleRotation, float& RollAngle) const;
	FVector CalculateAntiPitch(const FVector& AngularVelocity, const FRotator& VehicleRotation, float& PitchAngle) const;
	FVector CalculateAntiYaw(const FVector& AngularVelocity, UPrimitiveComponent* VehicleRoot, float& YawVelocity) const;
};
