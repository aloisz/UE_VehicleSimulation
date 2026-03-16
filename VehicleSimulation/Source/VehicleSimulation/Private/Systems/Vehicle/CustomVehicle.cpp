// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleSimulation/Public/Systems/Vehicle/CustomVehicle.h"
#include "Kismet/GameplayStatics.h"
#include "VehicleSimulation/Public/Systems/Vehicle/CustomVehicleMovementComponent.h"


ACustomVehicle::ACustomVehicle()
{
	PrimaryActorTick.bCanEverTick = true;
    
	VehicleRoot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleRoot"));
	RootComponent = VehicleRoot;
	VehicleRoot->SetSimulatePhysics(true);
	VehicleRoot->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    
	CenterOfMass = CreateDefaultSubobject<USceneComponent>(TEXT("CenterOfMass"));
	CenterOfMass->SetupAttachment(VehicleRoot);
	CenterOfMass->SetRelativeLocation(FVector(0, 0, -30));


	
	// Initialize Vehicle System Component
	VehicleMovement = CreateDefaultSubobject<UCustomVehicleMovementComponent>(TEXT("VehicleMovementComponent"));
	

	// ==== Wheels Initialize ====
	// Initialize wheels parents
	FL_Parent = CreateDefaultSubobject<USceneComponent>(TEXT("FL_Parent"));
	FR_Parent = CreateDefaultSubobject<USceneComponent>(TEXT("FR_Parent"));
	RL_Parent = CreateDefaultSubobject<USceneComponent>(TEXT("RL_Parent"));
	RR_Parent = CreateDefaultSubobject<USceneComponent>(TEXT("RR_Parent"));

	FL_Parent->SetupAttachment(VehicleRoot);
	FR_Parent->SetupAttachment(VehicleRoot);
	RL_Parent->SetupAttachment(VehicleRoot);
	RR_Parent->SetupAttachment(VehicleRoot);
	
	// Initialize wheels meshes
	FL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FL"));
	FR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FR"));
	RL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RL"));
	RR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RR"));

	FL->SetupAttachment(FL_Parent);
	FR->SetupAttachment(FR_Parent);
	RL->SetupAttachment(RL_Parent);
	RR->SetupAttachment(RR_Parent);
}

ACustomVehicle::~ACustomVehicle()
{
}


void ACustomVehicle::BeginPlay()
{
	Super::BeginPlay();
	CameraComponent = Cast<UCameraComponent>(GetDefaultSubobjectByName("FPVCam"));
	
	if (VehicleRoot && VehicleRoot->GetBodyInstance())
	{
		//VehicleMesh->GetBodyInstance()->SetMassOverride(VehicleMass, true);
		VehicleRoot->GetBodyInstance()->COMNudge = CenterOfMass->GetRelativeLocation();
		VehicleRoot->GetBodyInstance()->UpdateMassProperties();
	}
	
	// Fill the wheel meshes array
	WheelsMeshesParents.Add(FL_Parent);
	WheelsMeshesParents.Add(FR_Parent);
	WheelsMeshesParents.Add(RL_Parent);
	WheelsMeshesParents.Add(RR_Parent);

	WheelsMeshes = TArray<UStaticMeshComponent*>();
	WheelsMeshes.Add(FL);
	WheelsMeshes.Add(FR);
	WheelsMeshes.Add(RL);
	WheelsMeshes.Add(RR);

	
	// === Initialize Vehicle System Component ===
	VehicleMovement->InitializeSubsystems();
}

void ACustomVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


void ACustomVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACustomVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ACustomVehicle::MoveCamera(FVector2D Direction)
{
	/*float multiplier = PlayerController->IsUsingController() ?
	PlayerController->GetControllerSensitivityMultiplier() : 1.0f;
	if (!CameraComponent) return;
	FRotator CameraRotator = CameraComponent->GetRelativeRotation();
	CameraRotator.Yaw =FMath::Clamp(CameraRotator.Yaw + Direction.X * PlayerController->GetSensitivity(true) * multiplier,
		MinCameraAngleX, MaxCameraAngleX);
	CameraRotator.Pitch = FMath::Clamp(CameraRotator.Pitch + Direction.Y * PlayerController->GetSensitivity(false) * multiplier,
		MinCameraAngleY, MaxCameraAngleY);
	CameraRotator.Roll = 0;
	CameraComponent->SetRelativeRotation(CameraRotator);*/
}


USceneComponent* ACustomVehicle::GetCenterOfMass() const
{
	return CenterOfMass;
}

TArray<UStaticMeshComponent*> ACustomVehicle::GetWheelsMeshes() const
{
	return WheelsMeshes;
}

TArray<USceneComponent*> ACustomVehicle::GetWheelsParent() const
{
	return WheelsMeshesParents;
}

void ACustomVehicle::SetPlayerCloseEnough(bool value)
{
	bPlayerCanEnterDrivingPost = value;
}

void ACustomVehicle::SetCannotExitVehicle(bool value)
{
	bCannotExitVehicle = value;
}

bool ACustomVehicle::GetCannotExitVehicle()
{
	return bCannotExitVehicle;
}

void ACustomVehicle::SetIsVehicleStartingPossessedPawn(bool value) const
{
	if (value)
		VehicleMovement->InputSystem->EnteringVehicle();
	else VehicleMovement->InputSystem->ExitingVehicle();
}

void ACustomVehicle::SetPhysicsState(bool value)
{
	if (value == bIsVehiclePhysicsEnable) return;
	
	bIsVehiclePhysicsEnable = value;
	VehicleRoot->SetEnableGravity(bIsVehiclePhysicsEnable);
	VehicleRoot->SetSimulatePhysics(bIsVehiclePhysicsEnable);
}

bool ACustomVehicle::GetPhysicsState() const
{
	return bIsVehiclePhysicsEnable;
}

void ACustomVehicle::StartVehicleAtSpeed(float SpeedInKMH, float Duration)
{
	if (!VehicleRoot || !VehicleRoot->IsSimulatingPhysics())
	{
		return;
	}

	// Start the vehicle at this speed
	constexpr float KmHToCmS = 100000.f / 3600.f;
	const FVector Velocity =
		VehicleRoot->GetForwardVector() * SpeedInKMH * KmHToCmS;
	VehicleRoot->SetPhysicsLinearVelocity(Velocity, true);

	OnStartVehicleAtSpeedSignature.Broadcast(true);

	FTimerHandle CompletionTimer;
	GetWorld()->GetTimerManager().SetTimer(
		CompletionTimer,
		[this]()
		{
			OnStartVehicleAtSpeedSignature.Broadcast(false);
		},
		Duration,
		false
	);
}



UCustomVehicleMovementComponent* ACustomVehicle::GetVehicleMovement() const
{
	return VehicleMovement;
}


