// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleAerodynamicsComponent.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleEngineComponent.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleWheelSystemComponent.h"

// Sets default values for this component's properties
UVehicleAerodynamicsComponent::UVehicleAerodynamicsComponent()
{
}

// Called when the game starts
void UVehicleAerodynamicsComponent::BeginPlay()
{
    Super::BeginPlay();
}

// Called every frame
void UVehicleAerodynamicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVehicleAerodynamicsComponent::UpdateAerodynamics(float DeltaTime, UPrimitiveComponent* VehicleRoot, const USceneComponent* CenterOfMass) const
{
    if (!VehicleRoot) return;
    
    FVector Velocity = VehicleRoot->GetPhysicsLinearVelocity();
    float Speed = Velocity.Size();
    
    if (Speed < UVehicleWheelSystemComponent::EPSILON) return;
    
    FVector VelocityDirection = Velocity.GetSafeNormal();
    
    // Drag force 
    float DragMagnitude = 0.5f * DragCoefficient * Speed * Speed;
    FVector DragForce = -VelocityDirection * DragMagnitude;
    
    // Downforce
    float DownForceMagnitude = 0.5f * DownForce * Speed * Speed;
    FVector DownForceVector = -FVector::UpVector * DownForceMagnitude;
    
    // Apply aerodynamic forces at COM
    FVector CenterOfMassWorld = VehicleRoot->GetComponentLocation() + 
                               VehicleRoot->GetComponentRotation().RotateVector(CenterOfMass->GetRelativeLocation());
    
    if (!DragForce.ContainsNaN())
    {
        VehicleRoot->AddForceAtLocation(DragForce, CenterOfMassWorld);
    }
    
    if (!DownForceVector.ContainsNaN())
    {
        VehicleRoot->AddForceAtLocation(DownForceVector, CenterOfMassWorld);
    }
    
    if (bIsInDebug)
    {

        FVector DebugPosition = VehicleRoot->GetComponentLocation() + FVector(0, 0, 200);
        //drag force
        DrawDebugLine(GetWorld(), DebugPosition, DebugPosition + DragForce * 0.001f, 
                     FColor::Yellow, false, -1.0f, 0, 1.0f);
        //downforce
        DrawDebugLine(GetWorld(), DebugPosition, DebugPosition + DownForceVector * 0.001f, 
                     FColor::Magenta, false, -1.0f, 0, 1.0f);

        GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::White, 
            FString::Printf(TEXT("DragMagnitude: %.1f, DownForceMagnitude: %.1f"), 
            DragMagnitude, DownForceMagnitude));
    }
}


void UVehicleAerodynamicsComponent::UpdateStabilization(float DeltaTime, UPrimitiveComponent* VehicleRoot)
{
	if (!VehicleRoot) return;

    // === NO called on the CustomVehicleMovementComp Tick ===
    
    FVector AngularVelocity = VehicleRoot->GetPhysicsAngularVelocityInRadians();
    FRotator VehicleRotation = VehicleRoot->GetComponentRotation();

    float RollAngle = 0.0f;
    float PitchAngle = 0.0f;
    float YawVelocity = 0.0f;

    // Calculating Torque for roll, pitch, yaw
    FVector AntiRollTorque = CalculateAntiRoll(AngularVelocity, VehicleRotation, RollAngle);
    FVector AntiPitchTorque = CalculateAntiPitch(AngularVelocity, VehicleRotation, PitchAngle);
    FVector AntiYawTorque = CalculateAntiYaw(AngularVelocity, VehicleRoot, YawVelocity);
    
    // Combine all torques
    FVector TotalStabilizationTorque = AntiRollTorque + AntiPitchTorque + AntiYawTorque;
    if (!TotalStabilizationTorque.ContainsNaN() && !TotalStabilizationTorque.IsZero())
    {
        //VehicleRoot->AddTorqueInRadians(TotalStabilizationTorque);
    }
    
    if (bIsInDebug)
    {
        FVector CenterPos = VehicleRoot->GetComponentLocation();
        GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::White, 
            FString::Printf(TEXT("Roll: %.1f°, Pitch: %.1f°, Yaw Vel: %.1f rad/s"), 
            RollAngle, PitchAngle, YawVelocity));
    }
}


// reduce excessive roll
FVector UVehicleAerodynamicsComponent::CalculateAntiRoll(const FVector& AngularVelocity, const FRotator& VehicleRotation, float& RollAngle) const
{
   
    RollAngle = VehicleRotation.Roll;
    float RollVelocity = AngularVelocity.Y; // Roll 
    
    float AntiRollForce = -RollAngle * RollAngleMultiplier - RollVelocity * RollVelocityMultiplier;
    FVector AntiRollTorque = VehicleRotation.RotateVector(FVector(0, AntiRollForce, 0));
    return AntiRollTorque;
}

// reduce excessive pitch
FVector UVehicleAerodynamicsComponent::CalculateAntiPitch(const FVector& AngularVelocity, const FRotator& VehicleRotation, float& PitchAngle) const
{
    PitchAngle = VehicleRotation.Pitch;
    float PitchVelocity = AngularVelocity.X; // Pitch 
    
    float AntiPitchForce = -PitchAngle * PitchAngleMultiplier - PitchVelocity * PitchVelocityMultiplier;
    FVector AntiPitchTorque = VehicleRotation.RotateVector(FVector(AntiPitchForce, 0, 0));
    return AntiPitchTorque;
}

// prevent unwanted spinning
FVector UVehicleAerodynamicsComponent::CalculateAntiYaw(const FVector& AngularVelocity, UPrimitiveComponent* VehicleRoot, float& YawVelocity) const
{
    YawVelocity = AngularVelocity.Z; // Yaw
    FVector Velocity = VehicleRoot->GetPhysicsLinearVelocity();
    float Speed = Velocity.Size();
    
    // Only apply yaw damping at higher speeds to maintain maneuverability at low speeds
    float YawDamping = 0.0f;
    if (Speed * UVehicleEngineComponent::CONVERSION_TO_KMH > EnableYawDampingAtSpeed) // in km/h
    {
        YawDamping = -YawVelocity * YawVelocityMultiplier * (Speed / YawSpeedDivider); // Increase damping with speed
    }
    FVector AntiYawTorque = FVector(0, 0, YawDamping);
    return AntiYawTorque;
}

