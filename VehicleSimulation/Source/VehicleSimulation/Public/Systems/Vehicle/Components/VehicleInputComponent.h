// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnhancedInputSubsystems.h"
#include "VehicleInputComponent.generated.h"

class ACustomVehicle;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleInputComponentDisableEngine, bool , bIsEngineDisabled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleInputComponentToogleEngine, bool , bToggleEngine);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable )
class VEHICLESIMULATION_API UVehicleInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVehicleInputComponent();

	void UpdateSteerLag(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void SetThrottle(float Value);
	UFUNCTION(BlueprintCallable)
	void SetSteering(float Value);
	UFUNCTION(BlueprintCallable)
	void SetBrake(float Value);
	UFUNCTION(BlueprintCallable)
	void SetHandbrake(bool bEngaged);

	// === Engine === 
	UFUNCTION(BlueprintCallable)
	void SetToggleEngine(bool bRunning);
	UFUNCTION(BlueprintCallable)
	void DisableEngine(bool DisableEngine);


	UFUNCTION(BlueprintCallable)
	void HandleForwardBackward(UPrimitiveComponent* VehicleRoot, UVehicleEngineComponent* EngineComponent);
	UFUNCTION(BlueprintCallable)
	void SetRawThrottle(float Value) { RawThrottleInput = Value; }
	UFUNCTION(BlueprintCallable)
	void SetRawBrake(float Value) { RawBrakeInput = Value; }
	float GetRawThrottle() const { return RawThrottleInput; }
	float GetRawBrake() const { return RawBrakeInput; }

	UFUNCTION(BlueprintCallable)
	void EnteringVehicle();
	UFUNCTION(BlueprintCallable)
	void ExitingVehicle();

	UFUNCTION(BlueprintCallable)
	float GetThrottle();
	UFUNCTION(BlueprintCallable)
	float GetSteering();
	UFUNCTION(BlueprintCallable)
	float GetBrake();
	UFUNCTION(BlueprintCallable)
	bool GetToggleEngine();
	UFUNCTION(BlueprintCallable)
	bool GetIsEngineDisabled();
	UFUNCTION(BlueprintCallable)
	bool GetHandbrake();
	UFUNCTION(BlueprintCallable)
	void ResetInput();


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Restricted Action during Start Vehicle")
	TArray<UInputAction*> RestrictedActions;
	UFUNCTION()
	void OnStartVehicleAtSpeed(bool IsStillStartingVehicleAtSpeed);


	void OnComponentInitialized();
	
	UFUNCTION(BlueprintCallable)
	void DebugInput(float DeltaTime);
	
	UPROPERTY(BlueprintAssignable, Category = "Delegate | Engine")
	FOnVehicleInputComponentDisableEngine OnEngineIsDisable;

	UPROPERTY(BlueprintAssignable, Category = "Delegate | Engine")
	FOnVehicleInputComponentToogleEngine OnEngineToggle;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	float SteeringSmoothing = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	float ThrottleSmoothing = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	float BrakeSmoothing = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input | Engine")
	bool bEngineRunning = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input | Engine")
	bool bIsEngineDisabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	bool bIsVehicleOccupied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bIsInDebug = false;

private :
	float ThrottleInput = 0.0f;
	float SteerInput = 0.0f;
	float TargetSteerInput  = 0.0f;
	float BrakeInput = 0.0f;
	bool bHandbrakeEngaged = false;
	

private:
	float RawThrottleInput = 0.0f;
	float RawBrakeInput = 0.0f;

	static float SmoothInput(float Current, float Target, float Smoothing, float DeltaTime);

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
