// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VehicleSimulation/Public/Data/Structs/VehicleStruct.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleInputComponent.h"
#include "VehicleSimulation/Public/Data/Enums/VehicleEnums.h"
#include "Components/ActorComponent.h"
#include "VehicleWheelSystemComponent.generated.h"

class ACustomVehicle;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class VEHICLESIMULATION_API UVehicleWheelSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UVehicleWheelSystemComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                           FActorComponentTickFunction* ThisTickFunction) override;

    // === Init === 
    void InitializeWheels();
    void RegisterVehicleRoot(ACustomVehicle* Owner, UPrimitiveComponent* BaseVehicleRoot);
    
    // === Physics Update ===
    void UpdateWheelPhysics(float DeltaTime, UVehicleInputComponent* InputSystem);
    void UpdateTireForces(float DeltaTime, UVehicleInputComponent* InputSystem);
    void UpdateSuspension(float DeltaTime);
    void UpdateBraking(float DeltaTime, UVehicleInputComponent* InputSystem);

    // === Wheels ===
    TArray<FWheelData> GetWheels() const;
    FWheelData GetWheelData(int32 Index) const;
    

    // === Wheels configuration ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle", meta=(ClampMin="0", ClampMax="45"))
    float MaxSteerAngle = 40.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
    float BrakeForce = 3000.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
    float VehicleMass = 1500.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheels")
    TArray<FWheelData> Wheels;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
    TMap<ECustomSurfaceType, FSuspensionSettings> SuspensionSettings;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tires")
    UDataTable* TireSettingsData;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tires")
    TMap<ECustomSurfaceType, FTireSettings> TireSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bIsInDebug = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta=(EditCondition="bIsInDebug"))
    bool bDebugLongitudinal = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta=(EditCondition="bIsInDebug"))
    bool bDebugLateral = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta=(EditCondition="bIsInDebug"))
    bool bDebugWheel = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta=(EditCondition="bDebugWheel"))
    bool bDebugSuspension = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta=(EditCondition="bDebugWheel"))
    bool bDebugWheelDirection = false;


    // === Constants ===
    static constexpr float GRAVITY = 980.0f;
    static constexpr double EPSILON = 0.00001f;
    static constexpr float MAX_WHEEL_FORCE = 170000.0f;
protected:
    virtual void BeginPlay() override;
    
    UPROPERTY()
    TObjectPtr<UPrimitiveComponent> VehicleRoot;
    UPROPERTY()
    TObjectPtr<ACustomVehicle> VehicleOwner;

private:
    // === Steering System ===
    // calculates wheel coordinate system with steering applied
    FWheelCoordinateSystem CalculateWheelCoordinateSystem(int32 WheelIndex, float SteerInput) const;
    void DrawCylinderDebug(const FVector& Center, const FVector& AxisDirection, float Radius, float HalfHeight) const;
    void UpdateWheelsMeshesLocation(int WheelsIndex, const FVector& FinalWheelLocation) const;
    void UpdateWheelsMeshesRotation(int WheelsIndex, const FVector& WheelRight, float DeltaTime, UVehicleInputComponent* InputSystem);
    
    
    // Check if wheel should be steered
    bool IsSteerableWheel(int32 WheelIndex) const;
    
    // === Tire Physics ===
    void GetTireSettingsData();
    FVector CalculateTireForces(int32 WheelIndex, const FWheelCoordinateSystem& WheelCoords);
    void CalculateWheelSlip(int32 WheelIndex, const FWheelCoordinateSystem& WheelCoords, 
                           const FVector& WheelVelocity, float DeltaTime);
    float CalculateLongitudinalGrip(int WheelIndex, float SlipRatio, float NormalLoad, const FTireSettings& TireConfig) const;
    float CalculateLateralGrip(int WheelIndex, float SlipAngle, float NormalLoad, const FTireSettings& TireConfig) const;
    void ApplyFrictionClamps(int32 WheelIndex, const FWheelCoordinateSystem& WheelCoords, 
                            FVector& TireForce, float DeltaTime);
    
    // === Surface Detection ===
    ECustomSurfaceType DetectSurfaceType(const FHitResult& HitResult) const;
    FTireSettings GetTireSettingsForSurface(ECustomSurfaceType SurfaceType);
    FSuspensionSettings GetSuspensionSettingsForSurface(ECustomSurfaceType SurfaceType);
    
    // === Helpers ===
    FVector GetWheelWorldPosition(int32 WheelIndex) const;
    static float PacejkaFormula(float Slip, float B, float C, float D, float E);
    
    // Previous frame data for damping calculations
    TArray<float> PreviousSuspensionCompressions;
};
