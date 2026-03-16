// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleEngineComponent.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleEngineComponent.h"

#include "VehicleSimulation/Public/Systems/Vehicle/CustomVehicle.h"
#include "VehicleSimulation/Public/Data/Enums/VehicleEnums.h"
#include "VehicleSimulation/Public/Data/Structs/VehicleStruct.h"


// Sets default values for this component's properties
UVehicleEngineComponent::UVehicleEngineComponent()
{
	// Setup gear ratio
	Gearbox.Add(TEXT("R"), -2.92f);
	Gearbox.Add(TEXT("N"), 0.0f);
	Gearbox.Add(TEXT("1"), 2.50f);
	Gearbox.Add(TEXT("2"), 1.61f);
	Gearbox.Add(TEXT("3"), 1.10f);
	Gearbox.Add(TEXT("4"), 0.81f);
	Gearbox.Add(TEXT("5"), 0.68f);
}

void UVehicleEngineComponent::UpdateEnginePhysics(float DeltaTime, float ThrottleInput, float IsEngineRunning, UPrimitiveComponent* VehicleRoot)
{
	if (!VehicleRoot || !WheelSystem) return;

	CalculateActiveEnginePower(VehicleRoot);
	CalculateCurrentSpeed(VehicleRoot);
	HandleChangingGear(ThrottleInput);
	if (!IsEngineRunning) return;
	if (ApplyingEngineLimitation()) return;
    
	// Calculate average wheel angular velocity from driven wheels
	float AverageWheelAngularVelocity = 0.0f;
	int32 DrivenWheelCount = 0;
    
	for (int32 i = GetStartingWheelByTransmissionType(); i < GetEndingWheelByTransmissionType(); i++)
	{
		if (WheelSystem->GetWheels()[i].bIsGrounded)
		{
			AverageWheelAngularVelocity += WheelSystem->GetWheels()[i].WheelAngularVelocity;
			DrivenWheelCount++;
		}
	}
    
	if (DrivenWheelCount > 0)
	{
		AverageWheelAngularVelocity /= DrivenWheelCount;
	}
    
	// Update engine RPM and torque using differential equation
	UpdateEngineState(DeltaTime, ThrottleInput, AverageWheelAngularVelocity);
    
	// Apply torque => transmission to wheels
	ApplyTorqueToWheels(DeltaTime, CalculateEngineTorque(ThrottleInput), VehicleRoot);
}

float UVehicleEngineComponent::GetCurrentRPM() const
{
	if (VehicleOwner != nullptr )
	{
		if (VehicleOwner->GetVehicleMovement() != nullptr)
		{
			if (VehicleOwner->GetVehicleMovement()->InputSystem->GetToggleEngine()) return RPM;
		}
	}
	return 0.0f;
}

float UVehicleEngineComponent::GetCurrentSpeed() const
{
	return CurrentSpeed;
}


void UVehicleEngineComponent::SetEngineLimitation(bool IsLimited, float NewSpeed)
{
	bDoesEngineIsLimited = IsLimited;
	if (!bDoesEngineIsLimited) MaxSpeedAuthorized = 0.0f;
	else MaxSpeedAuthorized = NewSpeed;
}

bool UVehicleEngineComponent::ApplyingEngineLimitation() const
{
	if (bDoesEngineIsLimited)
	{
		if (GetCurrentSpeed() > MaxSpeedAuthorized) return true;
		return false;
	}
	
	return false;
}


void UVehicleEngineComponent::UpdateEngineState(float DeltaTime, float ThrottleInput, float WheelAngularVelocity)
{
	float CurrentGearRatio = GetGearRatio();
    
	if (FMath::IsNearlyZero(CurrentGearRatio))
	{
		// Neutral gear
		float TargetRPM = IdleRPM + (MaxRPM - IdleRPM) * FMath::Abs(ThrottleInput);
		RPM = FMath::FInterpTo(RPM, TargetRPM, DeltaTime, 5.0f);
		return;
	}
    
	// Convert RPM to angular velocity (rad/s)
	float EngineAngularVelocity = (RPM * 2.0f * PI) / 60.0f;
	
	float ExpectedWheelAngularVelocity = EngineAngularVelocity / CurrentGearRatio;
	float ExpectedEngineAngularVelocity = WheelAngularVelocity * CurrentGearRatio;
	
	float EngineTorqueInfluence = (OutputTorque / EngineInertia) * DeltaTime;
	float WheelCouplingTerm = WheelToEngineGain * (ExpectedEngineAngularVelocity - EngineAngularVelocity) * DeltaTime;
	
	EngineAngularVelocity += EngineTorqueInfluence + WheelCouplingTerm;
    
	// Clamp to engine limits
	float MinAngularVelocity = (IdleRPM * 2.0f * PI) / 60.0f;
	float MaxAngularVelocity = (MaxRPM * 2.0f * PI) / 60.0f;
	EngineAngularVelocity = FMath::Clamp(EngineAngularVelocity, MinAngularVelocity, MaxAngularVelocity);
    
	// Convert back to RPM
	RPM = (EngineAngularVelocity * 60.0f) / (2.0f * PI);
}

void UVehicleEngineComponent::ApplyTorqueToWheels(float DeltaTime, float EngineTorque, UPrimitiveComponent* VehicleRoot)
{
	float CurrentGearRatio = GetGearRatio();
    if (FMath::IsNearlyZero(CurrentGearRatio)) return; // Neutral
    
    // Torque after transmission
    float TransmissionTorque = EngineTorque * CurrentGearRatio;
    int32 DrivenWheelCount = 0;
    for (int32 i = GetStartingWheelByTransmissionType(); i < GetEndingWheelByTransmissionType(); i++)
    {
        if (WheelSystem->GetWheels()[i].bIsGrounded)
        {
            DrivenWheelCount++;
        }
    }
    
    if (DrivenWheelCount == 0) return;
    float WheelTorque = TransmissionTorque / DrivenWheelCount;
    
    for (int32 i = GetStartingWheelByTransmissionType(); i < GetEndingWheelByTransmissionType(); i++)
    {
        if (!WheelSystem->GetWheels()[i].bIsGrounded) continue;
        
        float WheelRadius = WheelSystem->GetWheels()[i].WheelRadius;
        float WheelForce = WheelTorque / WheelRadius;
    	
        FRotator VehicleRotation = VehicleRoot->GetComponentRotation();
        FVector ForwardDir = bIsGoingForward ? VehicleRotation.RotateVector(FVector::ForwardVector) : -VehicleRotation.RotateVector(FVector::BackwardVector);
        
        // Calculate drive force
        FVector DriveForce = ForwardDir * WheelForce * ActiveEnginePower; // add engine power to compensate the unreal units
        FVector WheelWorldPos = VehicleRoot->GetComponentLocation() + 
                               VehicleRotation.RotateVector(WheelSystem->GetWheels()[i].WheelOffset);
        
        // Apply force
        if (!DriveForce.ContainsNaN())
        {
            VehicleRoot->AddForceAtLocation(DriveForce, WheelWorldPos);
            
            // Update wheel angular velocity based on coupling
        	float EngineAngularVelocity = (RPM * 2.0f * PI) / 60.0f;
        	float ExpectedWheelAngularVelocity = EngineAngularVelocity / (CurrentGearRatio * DifferentialRatio);
        	
        	float CurrentWheelAngularVelocity = WheelSystem->GetWheels()[i].WheelAngularVelocity;
        	float NewWheelAngularVelocity = FMath::FInterpTo(CurrentWheelAngularVelocity, ExpectedWheelAngularVelocity, 
				DeltaTime,EngineToWheelGain);
        	
        	WheelSystem->GetWheels()[i].WheelAngularVelocity = NewWheelAngularVelocity;
        	UpdateWheelMeshesTorque(DeltaTime, i, WheelTorque);
        }
        
        if (bIsInDebug)
        {
            DrawDebugDirectionalArrow(GetWorld(), WheelWorldPos, WheelWorldPos + DriveForce * 0.01f, 
                10, FColor::Red, false, -1.0f, 0, 2.0f);
            
            GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::White, 
                FString::Printf(TEXT("Wheel[%d] Torque: %.1f Nm | Force: %.1f N"), 
                i, WheelTorque, WheelForce));
        }
    }

}

float UVehicleEngineComponent::CalculateEngineTorque(float ThrottleInput)
{
	float x = RPM;
	float a = TorqueCurveA;
	float b = TorqueCurveB;
	float c = TorqueCurveC;
	float d = TorqueCurveD;
	float f = TorqueCurveF;

	// Apply curve formula 
	float Exponent = -(c * (x - d)) * (c * (x - d)) / (f * f);
	float TorqueCurve = (a - b) * FMath::Exp(Exponent) + b;
    
	// Normalize RPM
	EngineRPMNormalized = FMath::Clamp(RPM / MaxRPM, 0.0f, 1.0f);
    
	// Apply throttle input
	OutputTorque = TorqueCurve * FMath::Abs(ThrottleInput);
    
	if (bIsInDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, 
			FString::Printf(TEXT("RPM: %.0f | Torque: %.1f Nm | TorqueCurve: %.1f"), 
			RPM, OutputTorque, TorqueCurve));
	}
    
	return OutputTorque;
}

void UVehicleEngineComponent::CalculateCurrentSpeed(UPrimitiveComponent* VehicleRoot)
{
	FVector Velocity = VehicleRoot->GetPhysicsLinearVelocity();
	CurrentSpeed = Velocity.Size() * CONVERSION_TO_KMH;
}

void UVehicleEngineComponent::UpdateWheelMeshesTorque(float DeltaTime, int WheelIndex, float WheelTorque)
{
	if (VehicleOwner->GetWheelsMeshes().Num() != WheelSystem->GetWheels().Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Wheel mesh count mismatch"));
		return;
	}
	
	FWheelData& Wheel = WheelSystem->GetWheels()[WheelIndex];
	UStaticMeshComponent* WheelMesh = VehicleOwner->GetWheelsMeshes()[WheelIndex];
    
	if (!WheelMesh) return;
    
	float RotationDeltaRad = Wheel.WheelAngularVelocity * DeltaTime;

	// Calculate Delta
	FQuat DeltaQuat = FQuat(WheelMesh->GetRightVector(), RotationDeltaRad / PI);
	FQuat CurrentQuat = WheelMesh->GetRelativeRotation().Quaternion();
    
	// Apply Delta
	FQuat NewQuat = CurrentQuat * DeltaQuat;
    
	// Set new rotation
	FQuat WheelQuat = WheelMesh->GetRelativeRotation().Quaternion();
	//WheelMesh->SetRelativeRotation(FQuat(WheelQuat.X, NewQuat.Y, WheelQuat.Z, NewQuat.W));
}

float UVehicleEngineComponent::GetGearRatio() const
{
	float Ratio = Gearbox.FindRef(ActualGear) * DifferentialRatio;
	if (bIsInDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, 
			FString::Printf(TEXT("Gear: %s | Ratio: %.2f | Speed: %.1f km/h | is going forward: %s"), 
			*ActualGear.ToString(), Ratio, CurrentSpeed, bIsGoingForward ? TEXT("true") : TEXT("false")));
	}
	
	return Ratio;
}

void UVehicleEngineComponent::ToggleGoingForward()
{
	bIsGoingForward = !bIsGoingForward;
}


void UVehicleEngineComponent::InitiateGearChange(int32 NewGear, bool IsImmediate)
{
	NextGearSelected = FName(*FString::FromInt(NewGear));
	bIsChangingGear = true;

	if (IsImmediate)
	{
		HandleChangingGearTimer();
	}
	else
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(
				ChangingGearTimerHandle,
				this,
				&UVehicleEngineComponent::HandleChangingGearTimer,
				ChangingGearTime,
				false
			);
		}
	}
}

bool UVehicleEngineComponent::ShouldShiftUp(int32 CurrentGear)
{
	int32 ThresholdIndex = CurrentGear - 1;
    
	if (ThresholdIndex < 0 || ThresholdIndex >= UpshiftThresholds.Num())
	{
		return false;
	}

	const TPair<float, float>& Threshold = UpshiftThresholds[ThresholdIndex];
	float SpeedThreshold = Threshold.Key;
	float RPMThreshold = Threshold.Value;
	
	return (CurrentSpeed >= SpeedThreshold && RPM >= RPMThreshold);
}

bool UVehicleEngineComponent::ShouldShiftDown(int32 CurrentGear)
{
	int32 ThresholdIndex = CurrentGear - 2;
    
	if (ThresholdIndex < 0 || ThresholdIndex >= DownshiftThresholds.Num())
	{
		return false;
	}

	const TPair<float, float>& Threshold = DownshiftThresholds[ThresholdIndex];
	float SpeedThreshold = Threshold.Key;
	float RPMThreshold = Threshold.Value;
	
	return (CurrentSpeed < SpeedThreshold || RPM < RPMThreshold);
}

void UVehicleEngineComponent::HandleChangingGear(float ThrottleInput)
{
	if (!bIsGoingForward)
	{
		ActualGear = FName("R");
		return;
	}
	
	if (bIsChangingGear)
	{
		return;
	}
	
	int32 CurrentGear = 0;
	FString GearString = ActualGear.ToString();
    
	if (GearString == "N" || GearString == "0" || GearString == "R")
	{
		InitiateGearChange(1, true);
		return;
	}
	else if (GearString != "R" ) CurrentGear = FCString::Atoi(*GearString);
	else return;
	
	// Check for upshift
	if (CurrentGear < MaxGear && ShouldShiftUp(CurrentGear))
	{
		InitiateGearChange(CurrentGear + 1, false);
	}
	// Check for downshift
	else if (CurrentGear > 1 && ShouldShiftDown(CurrentGear))
	{
		InitiateGearChange(CurrentGear - 1, false);
	}
}

void UVehicleEngineComponent::HandleChangingGearTimer()
{
	ActualGear = NextGearSelected;
	
	if (GetWorld())
	{
		FTimerHandle CompletionTimer;
		GetWorld()->GetTimerManager().SetTimer(
			CompletionTimer,
			[this]()
			{
				bIsChangingGear = false;
			},
			ChangingGearTime,
			false
		);
        
		GetWorld()->GetTimerManager().ClearTimer(ChangingGearTimerHandle);
	}
}

int UVehicleEngineComponent::GetStartingWheelByTransmissionType() const
{
	if (Transmission == ETransmissionType::AWD || Transmission == ETransmissionType::FWD) return 0;
	return 2;
}

int UVehicleEngineComponent::GetEndingWheelByTransmissionType() const
{
	if (Transmission == ETransmissionType::AWD || Transmission == ETransmissionType::RWD) return 4;
	return 2;
}

void UVehicleEngineComponent::CalculateActiveEnginePower(UPrimitiveComponent* VehicleRoot)
{
	ActiveEnginePower = 0;
	if (VehicleRoot->GetComponentRotation().Pitch >= ClimbingThresholdAngle)
	{
		FVector2D InputRange(ClimbingThresholdAngle, 180);
		ActiveEnginePower = EnginePower / FMath::GetMappedRangeValueClamped(InputRange, ClimbingOutputRange, VehicleRoot->GetComponentRotation().Pitch);
	}
	else
	{
		TMap<ECustomSurfaceType, FTireSettings>::ValueType TireSettings =
			WheelSystem->TireSettings[WheelSystem->GetWheelData(0).SurfaceType];
		
		if (TireSettings.bAffectEnginePower)
		{
			if (CurrentSpeed > TireSettings.VehicleSpeedNeededToEnableEnginePowerDivider)
				ActiveEnginePower = EnginePower / TireSettings.EnginePowerDivider;
			else ActiveEnginePower = EnginePower;
		}
		else ActiveEnginePower = EnginePower;
	}

	if (bIsInDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, 
			FString::Printf(TEXT("ActiveEnginePower: %.2f | ClimbingThresholdAngle: %.2f"), 
			ActiveEnginePower, VehicleRoot->GetComponentRotation().Pitch));
	}
}


// Called when the game starts
void UVehicleEngineComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


void UVehicleEngineComponent::RegisterWheelComponent(UVehicleWheelSystemComponent* WheelComp)
{
	WheelSystem = WheelComp;
}

void UVehicleEngineComponent::RegisterVehicleOwner(ACustomVehicle* Owner)
{
	VehicleOwner = Owner;
}

bool UVehicleEngineComponent::GetIsGoingForward() const
{
	return bIsGoingForward;
}

void UVehicleEngineComponent::SetIsGoingForward(bool NewIsGoingForward)
{
	bIsGoingForward = NewIsGoingForward;
}

// Called every frame
void UVehicleEngineComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

