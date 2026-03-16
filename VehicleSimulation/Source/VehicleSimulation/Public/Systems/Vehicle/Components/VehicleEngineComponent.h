// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VehicleWheelSystemComponent.h"
#include "Components/ActorComponent.h"
#include "VehicleSimulation/Public/Data/Enums/VehicleEnums.h"
#include "VehicleEngineComponent.generated.h"

// Forward declaration
class ACustomVehicle;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable )
class VEHICLESIMULATION_API UVehicleEngineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVehicleEngineComponent();

	void UpdateEnginePhysics(float DeltaTime, float ThrottleInput, float IsEngineRunning, UPrimitiveComponent* VehicleRoot);
	UFUNCTION(BlueprintCallable)
	float GetCurrentRPM() const;
	UFUNCTION(BlueprintCallable)
	float GetCurrentSpeed() const; // in km/h
	UFUNCTION(BlueprintCallable)
	void ToggleGoingForward();


	// === Max Speed Modifier ===
	UFUNCTION(BlueprintCallable)
	void SetEngineLimitation(bool IsLimited, float NewSpeed);
	bool ApplyingEngineLimitation() const;

	void RegisterWheelComponent(UVehicleWheelSystemComponent* WheelComp);
	void RegisterVehicleOwner(ACustomVehicle* Owner);
	
	bool GetIsGoingForward() const;
	void SetIsGoingForward(bool NewIsGoingForward);

	//=== Constants ===
	static constexpr float CONVERSION_TO_KMH = 0.036f;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// === Engine Properties ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Engine")
	float EnginePower = 7500.0f;
	float ActiveEnginePower; // power added to the calculation to compensate the unreal units
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Engine")
	float MaxRPM = 7000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Engine")
	float IdleRPM = 1000.0f;

	// === Transmission Properties ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	ETransmissionType Transmission = ETransmissionType::AWD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	bool bIsGoingForward = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transmission")
	TMap<FName, float> Gearbox;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	float ChangingGearTime = 1.7f; // How much time needed to change gear
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	float DifferentialRatio = 4.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	float EngineToWheelGain = 20.0f; // How strongly engine drives wheels
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	float WheelToEngineGain = 10.0f; // How strongly wheels affect engine

	// === Torque curve ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine|TorqueCurve")
	float TorqueCurveA = 700.0f; // Peak magnitude
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine|TorqueCurve")
	float TorqueCurveB = 100.0f; // Baseline torque
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine|TorqueCurve")
	float TorqueCurveC = 1.2f; // End slope
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine|TorqueCurve")
	float TorqueCurveD = 6000.0f; // Peak position (RPM)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine|TorqueCurve")
	float TorqueCurveF = 4000.0f; // End slope

	float OutputTorque;


	// === Physics ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine|Physics")
	float EngineInertia = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine|Debug")
	bool bIsInDebug = false;

	// === Engine State ===
	float RPM = 0.0f;
	float CurrentSpeed = 0.0f; // in km/h
	float EngineRPMNormalized = 0.0;

	// === Max Speed ===
	bool bDoesEngineIsLimited = false;
	float MaxSpeedAuthorized = 0.0f;

	// === Engine limitation when climbing Y axis ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine|ClimbingLimitation")
	float ClimbingThresholdAngle = 17;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine|ClimbingLimitation")
	FVector2D ClimbingOutputRange = FVector2D(2, 5);

private:
	void UpdateEngineState(float DeltaTime, float ThrottleInput, float WheelAngularVelocity);
	//float CalculateEngineRPM(float WheelSpeed, float ThrottleInput);
	void ApplyTorqueToWheels(float DeltaTime, float EngineTorque, UPrimitiveComponent* VehicleRoot);
	float CalculateEngineTorque(float ThrottleInput);
	void CalculateCurrentSpeed(UPrimitiveComponent* VehicleRoot);
	void UpdateWheelMeshesTorque(float DeltaTime, int WheelIndex, float WheelTorque);

	// === GearBox ===
	// Upshift thresholds
	TArray<TPair<float, float>> UpshiftThresholds = {
		{25.0f, 3000.0f},  // 1st -> 2nd
		{40.0f, 3500.0f},  // 2nd -> 3rd
		{60.0f, 3500.0f},  // 3rd -> 4th
		{80.0f, 3500.0f}   // 4th -> 5th
	};
	// Downshift thresholds
	TArray<TPair<float, float>> DownshiftThresholds = {
		{15.0f, 1500.0f},  // 2nd -> 1st
		{35.0f, 2000.0f},  // 3rd -> 2nd
		{45.0f, 2000.0f},  // 4th -> 3rd
		{65.0f, 2000.0f}   // 5th -> 4th
	};
	int32 MaxGear = 5;
	float GetGearRatio() const;
	FName ActualGear = TEXT("N");
	FName NextGearSelected = TEXT("N");
	bool bIsChangingGear = false;
	FTimerHandle ChangingGearTimerHandle;
	void InitiateGearChange(int32 NewGear, bool IsImmediate);
	bool ShouldShiftUp(int32 CurrentGear);
	bool ShouldShiftDown(int32 CurrentGear);
	void HandleChangingGear(float ThrottleInput);
	UFUNCTION()
	void HandleChangingGearTimer();
	
	// === Transmission ===
	int GetStartingWheelByTransmissionType() const;
	int GetEndingWheelByTransmissionType() const;

	// === EngineClimbingLimitation ===
	void CalculateActiveEnginePower(UPrimitiveComponent* VehicleRoot);

	UPROPERTY()
	TObjectPtr<UVehicleWheelSystemComponent> WheelSystem;
	UPROPERTY()
	TObjectPtr<ACustomVehicle> VehicleOwner;
	

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
