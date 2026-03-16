// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleSimulation/Public/Systems//Vehicle/CustomVehicleMovementComponent.h"
#include "VehicleSimulation/Public/Systems/Vehicle/CustomVehicle.h"

// Sets default values for this component's properties
UCustomVehicleMovementComponent::UCustomVehicleMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics; // ensure we apply forces before physics simulation

	// Create subsystems
	WheelSystem = CreateDefaultSubobject<UVehicleWheelSystemComponent>(TEXT("WheelSystem"));
	EngineSystem = CreateDefaultSubobject<UVehicleEngineComponent>(TEXT("EngineSystem"));
	InputSystem = CreateDefaultSubobject<UVehicleInputComponent>(TEXT("InputSystem"));
	AerodynamicsSystem = CreateDefaultSubobject<UVehicleAerodynamicsComponent>(TEXT("AerodynamicsSystem"));

	// Disable their ticking
	if (WheelSystem)
		WheelSystem->PrimaryComponentTick.bCanEverTick = false;
	if (EngineSystem)
		EngineSystem->PrimaryComponentTick.bCanEverTick = false;
	if (AerodynamicsSystem)
		AerodynamicsSystem->PrimaryComponentTick.bCanEverTick = false;
}


void UCustomVehicleMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCustomVehicleMovementComponent::UpdatePhysicsState() const
{
	FVector VehicleVelocity = VehicleRoot->GetPhysicsLinearVelocity();
	float VehicleSpeed = VehicleVelocity.Size();
	VehicleSpeed *= UVehicleEngineComponent::CONVERSION_TO_KMH;
	
	for (auto Wheel : WheelSystem->GetWheels())
	{
		if (Wheel.bIsGrounded)
		{
			if (InputSystem->GetHandbrake())
			{
				if (VehicleSpeed < 0.2f)
				{
					if (!VehicleRoot->IsSimulatingPhysics()) return;
					VehicleRoot->SetSimulatePhysics(false);
				}
			}
			else
			{
				if (VehicleRoot->IsSimulatingPhysics()) return;
				VehicleRoot->SetSimulatePhysics(true);
			}
		}
	}

	
}

// Called every frame
void UCustomVehicleMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                    FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsInitialized) return;
	if (!EngineSystem || !WheelSystem || !InputSystem || !AerodynamicsSystem || !VehicleOwner || !VehicleRoot)
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleMovement: Missing subsystem"));
		return;
	}
	
	if (!VehicleOwner->GetPhysicsState()) return;
	
	// Update Physics state
	UpdatePhysicsState();
	if (!VehicleRoot->IsSimulatingPhysics()) return;

	
	// Process input
	float Throttle = InputSystem->GetThrottle();
	float Steering = InputSystem->GetSteering();
	float Brake = InputSystem->GetBrake();
	bool bHandbrake = InputSystem->GetHandbrake();
	InputSystem->UpdateSteerLag(DeltaTime);
	InputSystem->HandleForwardBackward(VehicleRoot, EngineSystem);
	InputSystem->DebugInput(DeltaTime);
	
	// Update engine physics
	EngineSystem->UpdateEnginePhysics(DeltaTime, Throttle, InputSystem->GetToggleEngine(), VehicleRoot);

	// Update wheel physics
	WheelSystem->UpdateWheelPhysics(DeltaTime, InputSystem);
	WheelSystem->UpdateSuspension(DeltaTime);
	WheelSystem->UpdateTireForces(DeltaTime, InputSystem);
	WheelSystem->UpdateBraking(DeltaTime, InputSystem);
	
	// Update Aerodynamics
	AerodynamicsSystem->UpdateAerodynamics(DeltaTime, VehicleRoot, CenterOfMass);
	// Not called because cause issue of shacking the vehicle
	AerodynamicsSystem->UpdateStabilization(DeltaTime, VehicleRoot);
}

void UCustomVehicleMovementComponent::InitializeSubsystems()
{
	VehicleOwner = Cast<ACustomVehicle>(GetOwner());
	if (!VehicleOwner)
	{
		UE_LOG(LogTemp, Error, TEXT("CustomVehicleMovementComponent: Owner is not a vehicle pawn!"));
		return;
	}

	VehicleRoot = Cast<UStaticMeshComponent>(VehicleOwner->GetRootComponent());
	if (!VehicleRoot)
	{
		UE_LOG(LogTemp, Error, TEXT("CustomVehicleMovementComponent: No root found on vehicle!"));
		return;
	}

	CenterOfMass = VehicleOwner->GetCenterOfMass();
	if (!CenterOfMass)
	{
		UE_LOG(LogTemp, Error, TEXT("No CenterOfMass found on vehicle!"));
		return;
	}

	if (InputSystem)
	{
		InputSystem->SetToggleEngine(true);
	}

	if (!WheelSystem)
	{
		UE_LOG(LogTemp, Error, TEXT("UVehicleWheelSystemComponent: is not set!"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("VehicleRoot name is %s"), *VehicleRoot->GetName());

	if (EngineSystem)
	{
		EngineSystem->RegisterWheelComponent(WheelSystem);
		EngineSystem->RegisterVehicleOwner(VehicleOwner);
	}
	if (WheelSystem)
	{
		WheelSystem->RegisterVehicleRoot(VehicleOwner, VehicleRoot);
	}

	bIsInitialized = true;
}

void UCustomVehicleMovementComponent::ResetVehicleMovement()
{
	VehicleRoot->ResetSceneVelocity();
	VehicleRoot->SetPhysicsLinearVelocity(FVector::ZeroVector);
}

