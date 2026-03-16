// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleInputComponent.h"
#include "VehicleSimulation/Public/Systems/Vehicle/CustomVehicle.h"
#include "Kismet/GameplayStatics.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleEngineComponent.h"

//#include <ThirdParty/ShaderConductor/ShaderConductor/External/DirectXShaderCompiler/include/dxc/DXIL/DxilConstants.h>

//#include "LightmapResRatioAdjust.h"


// Sets default values for this component's properties
UVehicleInputComponent::UVehicleInputComponent()
{
}

void UVehicleInputComponent::UpdateSteerLag(float DeltaTime)
{
	// Steer with lag 
	SteerInput = FMath::FInterpTo(SteerInput, TargetSteerInput, DeltaTime, SteeringSmoothing);
}

void UVehicleInputComponent::SetThrottle(float Value)
{
	if (!bEngineRunning) ThrottleInput = 0.0f;
	else ThrottleInput = FMath::Clamp(Value, 0.0f, 1.0f);
}

void UVehicleInputComponent::SetSteering(float Value)
{
	TargetSteerInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void UVehicleInputComponent::SetBrake(float Value)
{
	BrakeInput = FMath::Clamp(Value, 0.0f, 1.0f);
}

void UVehicleInputComponent::HandleForwardBackward(UPrimitiveComponent* VehicleRoot, UVehicleEngineComponent* EngineComponent)
{
	// Get vehicle velocity
    FVector VehicleVelocity = VehicleRoot->GetPhysicsLinearVelocity();
    FVector ForwardVector = VehicleRoot->GetForwardVector();
    float DirectionalSpeed = FVector::DotProduct(VehicleVelocity, ForwardVector);
	
    DirectionalSpeed *= UVehicleEngineComponent::CONVERSION_TO_KMH;
    const float StopThreshold = 1.0f;
	
    float RawThrottle = GetRawThrottle();
    float RawBrake = GetRawBrake();
	
    if (DirectionalSpeed > StopThreshold)
    {
        // Moving forward
    	EngineComponent->SetIsGoingForward(true);
        SetThrottle(RawThrottle);
        SetBrake(RawBrake);
    }
    else if (DirectionalSpeed < -StopThreshold)
    {
    	// Moving backward
    	EngineComponent->SetIsGoingForward(false);
        SetThrottle(RawBrake);
        SetBrake(RawThrottle);
    }
    else
    {
        // nearly stopped => allow direction change based on input
        if (RawThrottle > 0.1f)
        {
            // Player wants to go forward
        	EngineComponent->SetIsGoingForward(true);
            SetThrottle(RawThrottle);
            SetBrake(0.0f);
        }
        else if (RawBrake > 0.1f)
        {
            // Player wants to go backward
        	EngineComponent->SetIsGoingForward(false);
            SetThrottle(RawBrake);
            SetBrake(0.0f);
        }
        else
        {
            // No input
            SetThrottle(0.0f);
            SetBrake(0.0f);
        }
    }

	if (bIsInDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, 
			FString::Printf(TEXT("Dir: %s | Speed: %.2f km/h"), 
			EngineComponent->GetIsGoingForward() ? TEXT("FWD") : TEXT("REV"), DirectionalSpeed));
	}
}

void UVehicleInputComponent::SetHandbrake(bool bEngaged)
{
	bHandbrakeEngaged = bEngaged;
}

void UVehicleInputComponent::SetToggleEngine(bool bRunning)
{
	if (bIsEngineDisabled) return;
	
	bEngineRunning = bRunning;
	SetHandbrake(!bEngineRunning);
	OnEngineToggle.Broadcast(bEngineRunning);
}

void UVehicleInputComponent::DisableEngine(bool DisableEngine)
{
	OnEngineIsDisable.Broadcast(DisableEngine);
	if (DisableEngine)
		SetToggleEngine(!DisableEngine);
	
	bIsEngineDisabled = DisableEngine;
}

void UVehicleInputComponent::EnteringVehicle()
{
	bIsVehicleOccupied = true;
	SetRawThrottle(0.0f);
	SetRawBrake(0.0f);
	
	if (bEngineRunning)
	SetHandbrake(false);
}

void UVehicleInputComponent::ExitingVehicle()
{
	bIsVehicleOccupied = false;
	SetRawThrottle(0.0f);
	SetRawBrake(0.0f);
	
	SetHandbrake(true);
}

float UVehicleInputComponent::GetThrottle()
{
	return ThrottleInput;
}

float UVehicleInputComponent::GetSteering()
{
	return SteerInput;
}

float UVehicleInputComponent::GetBrake()
{
	return BrakeInput;
}

bool UVehicleInputComponent::GetToggleEngine()
{
	return bEngineRunning;
}

bool UVehicleInputComponent::GetIsEngineDisabled()
{
	return bIsEngineDisabled;
}

bool UVehicleInputComponent::GetHandbrake()
{
	return bHandbrakeEngaged;
}

void UVehicleInputComponent::ResetInput()
{
	SetRawBrake(0.f);
	SetBrake(0.f);
	SetThrottle(0.f);
	SetRawThrottle(0.f);
}

void UVehicleInputComponent::DebugInput(float DeltaTime)
{
	if (bIsInDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::White, 
			FString::Printf(TEXT("GetThrottle: %f | GetSteering: %f | GetBrake: %f | GetHandbrake: %hdd | GetToggleEngine: %hdd"), 
			GetThrottle(), GetSteering(), GetBrake(), GetHandbrake(), GetToggleEngine()));
	}
}


// Called when the game starts
void UVehicleInputComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UVehicleInputComponent::OnStartVehicleAtSpeed(bool IsStillStartingVehicleAtSpeed)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (IsStillStartingVehicleAtSpeed)
	{
		for (auto action : RestrictedActions)
		{
			SetRawThrottle(1);
		}
	}
	else
	{
		SetRawThrottle(0);
	}
	
}

void UVehicleInputComponent::OnComponentInitialized()
{
}


float UVehicleInputComponent::SmoothInput(float Current, float Target, float Smoothing, float DeltaTime)
{
	return FMath::SmoothStep(Current, Target, Smoothing * DeltaTime);
}

// Called every frame
void UVehicleInputComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

