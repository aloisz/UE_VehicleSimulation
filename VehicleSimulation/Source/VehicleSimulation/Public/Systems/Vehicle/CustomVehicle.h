
#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "VehicleSimulation/Public/Data/Enums/VehicleEnums.h"
#include "VehicleSimulation/Public/Systems/Vehicle/CustomVehicleMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "CustomVehicle.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartVehicleAtSpeedSignature, bool, IsStillStartingVehicleAtSpeed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnterExitVehicle, bool, EnteringVehicle);

UCLASS()
class VEHICLESIMULATION_API ACustomVehicle : public APawn
{
	GENERATED_BODY()

public:
	ACustomVehicle();
	~ACustomVehicle();
	

	USceneComponent* GetCenterOfMass() const;
	// === Wheels ===
	TArray<UStaticMeshComponent*> GetWheelsMeshes() const;
	TArray<USceneComponent*> GetWheelsParent() const;

	// only for doors interactions
	UFUNCTION(BlueprintCallable)
	void SetPlayerCloseEnough(bool value);
	
	UFUNCTION(BlueprintCallable)
	void SetCannotExitVehicle(bool value);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetCannotExitVehicle();
	void SetIsVehicleStartingPossessedPawn(bool value) const;

	UFUNCTION(BlueprintCallable)
	void SetPhysicsState(bool value);
	UFUNCTION(BlueprintCallable)
	bool GetPhysicsState() const;

	//
	// The vehicle will be at this speed in km/h when called
	UFUNCTION(BlueprintCallable)
	void StartVehicleAtSpeed(float SpeedInKMH, float Duration);

	UPROPERTY(BlueprintAssignable, Category = "Vehicle Delegate")
	FOnStartVehicleAtSpeedSignature OnStartVehicleAtSpeedSignature;
	
	UPROPERTY(BlueprintAssignable, Category = "Vehicle Delegate")
	FOnEnterExitVehicle OnEnterExitVehicle;

	UFUNCTION(BlueprintCallable)
	UCustomVehicleMovementComponent* GetVehicleMovement() const;


protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Vehicle | Interactions")
	bool bCannotExitVehicle = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinCameraAngleX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxCameraAngleX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinCameraAngleY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxCameraAngleY;
	
	// enable / disable physics
	bool bIsVehiclePhysicsEnable = false; 
	
	UPROPERTY()
	TObjectPtr<UCameraComponent> CameraComponent;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Vehicle Components
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* VehicleRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* CenterOfMass;


	// === Component ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UCustomVehicleMovementComponent* VehicleMovement;
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wheels")
	bool bIsWheelHiddenWhenDriving = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheels")
	TArray<UStaticMeshComponent*> WheelsMeshes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheels")
	TArray<USceneComponent*> WheelsMeshesParents;
	
	// Wheels Meshs parent => Yaw Rotation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wheels")
	USceneComponent* FL_Parent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wheels")
	USceneComponent* FR_Parent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wheels")
	USceneComponent* RL_Parent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wheels")
	USceneComponent* RR_Parent;


	// Wheels Meshs => Roll Rotation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wheels")
	UStaticMeshComponent* FL;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wheels")
	UStaticMeshComponent* FR;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wheels")
	UStaticMeshComponent* RL;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wheels")
	UStaticMeshComponent* RR;

	
	UFUNCTION(BlueprintCallable)
	void MoveCamera(FVector2D Direction);

#pragma region Pawn Logic
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bPlayerCanEnterDrivingPost;
#pragma endregion


private:
};