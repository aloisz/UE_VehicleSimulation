#pragma once

#include "CoreMinimal.h"
#include "VehicleSimulation/Public/Data/Enums/VehicleEnums.h"
#include "Engine/DataTable.h"
#include "Engine/DataAsset.h"

#include "VehicleStruct.generated.h"

class VehicleStruct
{
public:
	
};


USTRUCT()
struct FWheelCoordinateSystem
{
	GENERATED_BODY()
    
	FVector ForwardDir = FVector::ForwardVector;
	FVector RightDir = FVector::RightVector;
	FVector UpDir = FVector::UpVector;
	float SteerAngleRad = 0.0f;
};

USTRUCT(BlueprintType)
struct FWheelData 
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector WheelOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WheelRadius = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WheelWidth = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CylinderTracePoints = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuspensionLength = 50.0f;

	// Physics State
	bool bIsGrounded = false;
	FVector HitPoint = FVector::ZeroVector;
	FVector HitNormal = FVector::UpVector;
	ECustomSurfaceType SurfaceType = ECustomSurfaceType::Road;
	float SuspensionCompression = 0.0f;

	// Wheel Physics
	float WheelAngularVelocity = 0.0f; // rad/s
	float WheelLoad = 0.0f; // Normal force from suspension
	float SlipRatio = 0.0f; // Longitudinal slip
	float SlipAngle = 0.0f; // Lateral slip angle in radians
	float CamberAngle = 0.0f;

	// Tire force
	FVector TireForce = FVector::ZeroVector;
	float LongitudinalForce = 0.0f;
	float LateralForce = 0.0f;

	FWheelData()
	{
		WheelOffset = FVector::ZeroVector;
		WheelRadius = 35.0f;
		SuspensionLength = 50.0f;
		bIsGrounded = false;
		SuspensionCompression = 0.0f;
		WheelAngularVelocity = 0.0f;
		WheelLoad = 0.0f;
		SlipRatio = 0.0f;
		SlipAngle = 0.0f;
		CamberAngle = 0.0f;
		TireForce = FVector::ZeroVector;
		LongitudinalForce = 0.0f;
		LateralForce = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct FSuspensionSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
	float SpringStrength = 45000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
	float DamperStrength = 3500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
	float MaxSuspensionForce = 60000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float RestLength = 0.7f; // Rest suspension position
};

USTRUCT(BlueprintType)
struct FTireSettings : public FTableRowBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire")
	float PeakSlipRatio = 0.15f; // slip for maximum grip

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire")
	float PeakSlipAngle = 8.0f; // in degrees

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire")
	float PeakLongitudinalGrip = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire")
	float PeakLateralGrip = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire")
	float FrictionCoefficient = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire")
	float LoadSensitivity = 0.8f;


	// === Pacjeka Formula ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Pacjeka Formula",meta=(ToolTip="Controls curve slope near zero slip"))
	float B_Lateral = 5.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Pacjeka Formula", meta=(ToolTip="Adjusts peak sharpness"))
	float C_Lateral = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Pacjeka Formula", meta=(ToolTip="Adjusts curve magnitude"))
	float D_Lateral = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Pacjeka Formula", meta=(ToolTip="Adjusts ends slope"))
	float E_Lateral = 0.97f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Pacjeka Formula", meta=(ToolTip="Controls curve slope near zero slip"))
	float B_Longitudinal = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Pacjeka Formula", meta=(ToolTip="Adjusts peak sharpness"))
	float C_Longitudinal = 1.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Pacjeka Formula", meta=(ToolTip="Adjusts curve magnitude"))
	float D_Longitudinal = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Pacjeka Formula", meta=(ToolTip="Adjusts ends slope"))
	float E_Longitudinal = 0.97f;


	// === Engine ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine", meta=(ToolTip="Controls if the engine gets affected by the surface type"))
	bool bAffectEnginePower = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine", meta=(ToolTip="Controls how the engine is affected by the surface type"))
	float EnginePowerDivider = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine", meta=(ToolTip="Defines the minimum speed required for the engine power divider to become active."))
	float VehicleSpeedNeededToEnableEnginePowerDivider = 10.0f;
};