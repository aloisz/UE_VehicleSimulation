// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleAerodynamicsComponent.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleEngineComponent.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleInputComponent.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleWheelSystemComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "CustomVehicleMovementComponent.generated.h"


// Forward declaration
class ACustomVehicle;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class VEHICLESIMULATION_API UCustomVehicleMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
public:
	UCustomVehicleMovementComponent();

	UPROPERTY()
	TObjectPtr<ACustomVehicle> VehicleOwner;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "VehicleRoot")
	UStaticMeshComponent* VehicleRoot;

	bool bIsInitialized = false;

	UPROPERTY()
	USceneComponent* CenterOfMass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UVehicleWheelSystemComponent> WheelSystem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UVehicleEngineComponent> EngineSystem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UVehicleInputComponent> InputSystem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UVehicleAerodynamicsComponent> AerodynamicsSystem;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void UpdatePhysicsState() const;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	void InitializeSubsystems();
	void ResetVehicleMovement(); // Reset all Component 
};
