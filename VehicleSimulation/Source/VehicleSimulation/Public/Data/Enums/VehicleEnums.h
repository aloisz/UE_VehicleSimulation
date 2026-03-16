#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "VehicleEnums.generated.h"

class VehicleEnums
{
public:

	
};



UENUM(BlueprintType)
enum class ECustomSurfaceType : uint8
{
	Road  UMETA(DisplayName = "Road"),
	Grass UMETA(DisplayName = "Grass"),
	Mud   UMETA(DisplayName = "Mud"),
	Water UMETA(DisplayName = "Water"),
	Rock  UMETA(DisplayName = "Rock"),
};

UENUM(BlueprintType)
enum class ETransmissionType : uint8
{
	FWD UMETA(DisplayName = "FWD"), // Traction
	RWD UMETA(DisplayName = "RWD"), // Propulsion
	AWD UMETA(DisplayName = "AWD") //4X4 
};