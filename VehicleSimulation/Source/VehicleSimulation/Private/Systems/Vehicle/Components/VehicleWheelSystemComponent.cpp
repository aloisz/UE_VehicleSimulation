// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleWheelSystemComponent.h"

#include "VehicleSimulation/Public/Systems/Vehicle/CustomVehicle.h"
#include "VehicleSimulation/Public/Systems/Vehicle/Components/VehicleInputComponent.h"
#include "VehicleSimulation/Public/Systems/Vehicle/CustomVehicle.h"

UVehicleWheelSystemComponent::UVehicleWheelSystemComponent()
{
    InitializeWheels();
}

void UVehicleWheelSystemComponent::BeginPlay()
{
    Super::BeginPlay();

    GetTireSettingsData();
}

void UVehicleWheelSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVehicleWheelSystemComponent::InitializeWheels()
{
    // Initialize Wheels
    Wheels.SetNum(4);
    Wheels[0].WheelOffset = FVector(120, -80, -100);  // Front Left
    Wheels[1].WheelOffset = FVector(120, 80, -100);   // Front Right
    Wheels[2].WheelOffset = FVector(-120, -80, -100); // Rear Left
    Wheels[3].WheelOffset = FVector(-120, 80, -100);  // Rear Right
    
    // Initialize Suspension Settings
    FSuspensionSettings RoadSuspension;
    RoadSuspension.SpringStrength = 45000.0f;
    RoadSuspension.DamperStrength = 3500.0f;
    RoadSuspension.MaxSuspensionForce = 60000.0f;
    RoadSuspension.RestLength = 0.7f;
    SuspensionSettings.Add(ECustomSurfaceType::Road, RoadSuspension);
    
    /*// Initialize Tire Settings
    FTireSettings RoadTires;
    RoadTires.PeakSlipRatio = 0.15f;
    RoadTires.PeakSlipAngle = 8.0f;
    RoadTires.PeakLongitudinalGrip = 1.0f;
    RoadTires.PeakLateralGrip = 1.2f;
    RoadTires.FrictionCoefficient = 0.8f;
    RoadTires.LoadSensitivity = 0.8f;
    TireSettings.Add(ECustomSurfaceType::Road, RoadTires);*/
    
    // Initialize arrays
    PreviousSuspensionCompressions.SetNum(4);
    for (int32 i = 0; i < 4; i++)
    {
        PreviousSuspensionCompressions[i] = 0.0f;
    }
}

void UVehicleWheelSystemComponent::RegisterVehicleRoot(ACustomVehicle* Owner, UPrimitiveComponent* BaseVehicleRoot)
{
    VehicleOwner = Owner;
    VehicleRoot = BaseVehicleRoot;
}

// ========================================
#pragma region Steering

FWheelCoordinateSystem UVehicleWheelSystemComponent::CalculateWheelCoordinateSystem(
    int32 WheelIndex, float SteerInput) const
{
    FWheelCoordinateSystem WheelCoords;
    
    if (!VehicleRoot)
    {
        return WheelCoords;
    }
    
    FRotator VehicleRotation = VehicleRoot->GetComponentRotation();
    WheelCoords.ForwardDir = VehicleRotation.RotateVector(FVector::ForwardVector);
    WheelCoords.RightDir = VehicleRotation.RotateVector(FVector::RightVector);
    WheelCoords.UpDir = VehicleRotation.RotateVector(FVector::UpVector);
    
    // Apply steering only to steerable wheels
    if (IsSteerableWheel(WheelIndex))
    {
        WheelCoords.SteerAngleRad = FMath::DegreesToRadians(SteerInput * MaxSteerAngle);
        
        // Rotate forward and right directions by steering angle
        WheelCoords.ForwardDir = WheelCoords.ForwardDir * FMath::Cos(WheelCoords.SteerAngleRad) + 
                                 WheelCoords.RightDir * FMath::Sin(WheelCoords.SteerAngleRad);
        WheelCoords.RightDir = FVector::CrossProduct(WheelCoords.ForwardDir, WheelCoords.UpDir);
    }
    
    return WheelCoords;
}

bool UVehicleWheelSystemComponent::IsSteerableWheel(int32 WheelIndex) const
{
    // Front wheels 0 & 1 are steerable
    return WheelIndex < 2;
}

void UVehicleWheelSystemComponent::GetTireSettingsData()
{
    // add the TireSettings data to the Tire Settings
    if (TireSettingsData)
    {
        if (TireSettingsData->GetRowNames().Num() <= 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Data table is empty"));
            return;
        }

        // TODO : Fix le get du row name => pas assez safe pour le moment
        FTireSettings* RoadTire = TireSettingsData->FindRow<FTireSettings>(TEXT("Road"), "", true);
        FTireSettings* GrassTire = TireSettingsData->FindRow<FTireSettings>(TEXT("Grass"), "", true);
        if (RoadTire && GrassTire)
        {
            TireSettings.Add(ECustomSurfaceType::Road, *RoadTire);
            TireSettings.Add(ECustomSurfaceType::Grass, *GrassTire);
        }
        
    }
}

FVector UVehicleWheelSystemComponent::GetWheelWorldPosition(int32 WheelIndex) const
{
    if (!VehicleRoot || !Wheels.IsValidIndex(WheelIndex))
    {
        return FVector::ZeroVector;
    }
    
    return VehicleRoot->GetComponentLocation() + 
           VehicleRoot->GetComponentRotation().RotateVector(Wheels[WheelIndex].WheelOffset);
}

#pragma endregion
// ========================================

// ========================================
#pragma region Wheel Physics
void UVehicleWheelSystemComponent::UpdateWheelPhysics(float DeltaTime, UVehicleInputComponent* InputSystem)
{
    if (!VehicleRoot || !InputSystem) return;
    
    FVector VehicleLocation = VehicleRoot->GetComponentLocation();
    FRotator VehicleRotation = VehicleRoot->GetComponentRotation();
    float SteerInput = InputSystem->GetSteering();

    for (int32 i = 0; i < Wheels.Num(); i++)
    {
        FVector WheelWorldPos = GetWheelWorldPosition(i);

        // Create TArray for hit results
        TArray<FHitResult> OutHits;
        
        FVector TraceStart = WheelWorldPos + FVector(0, 0, Wheels[i].WheelRadius);
        FVector TraceEnd = WheelWorldPos - FVector(0, 0, Wheels[i].SuspensionLength); // OLD + Wheels[i].WheelRadius

        // Calculate wheel coordinate system for proper orientation
        FWheelCoordinateSystem WheelCoords = CalculateWheelCoordinateSystem(i, SteerInput);
        
        FVector WheelRight = WheelCoords.RightDir;
        FVector WheelForward = WheelCoords.ForwardDir;
        
        FCollisionQueryParams QueryParams;
        QueryParams.bReturnPhysicalMaterial = true;
        QueryParams.AddIgnoredActor(VehicleRoot->GetOwner());

        bool isHit = false;
        float SmallSphereRadius = Wheels[i].WheelRadius * 0.15f;
        float StepSize = Wheels[i].WheelRadius / static_cast<float>(Wheels[i].CylinderTracePoints - 1);

        FVector Up = FVector::UpVector;
        FVector Right = FVector::CrossProduct(WheelRight, Up).GetSafeNormal();
        for (int32 j = 0; j < Wheels[i].CylinderTracePoints; j++)
        {
            float Angle = -1 * (PI / (Wheels[i].CylinderTracePoints - 1)) * j;
            FVector Offset = (FMath::Cos(Angle) * Right + FMath::Sin(Angle) * Up) * Wheels[i].WheelRadius;

            FVector LocalTraceStart = TraceStart + Offset;
            FVector LocalTraceEnd = (TraceEnd + Offset);

            FCollisionShape SmallSphere = FCollisionShape::MakeSphere(SmallSphereRadius);
            
            TArray<FHitResult> LocalHits;
            bool localHit = GetWorld()->SweepMultiByChannel(LocalHits, LocalTraceStart, LocalTraceEnd, 
                                                            FQuat::Identity, ECC_Visibility, SmallSphere, QueryParams);
            if (localHit)
            {
                isHit = true;
                OutHits.Append(LocalHits);
            }
            // Debug visualization for each trace point
            if (bIsInDebug && bDebugWheel)
            {
                DrawDebugLine(GetWorld(), LocalTraceStart, LocalTraceEnd, 
                             localHit ? FColor::Green : FColor::Red, false, -1.0f, 0, 0.5f);
                DrawDebugSphere(GetWorld(), LocalTraceStart, SmallSphereRadius, 8, 
                                   localHit ? FColor::Green : FColor::Yellow, false, -1.0f, 0, 0.5f);
            }
        }
        
        if (isHit)
        {
            // Find the closest hit
            FHitResult* ClosestHit = nullptr;
            float ClosestDistance = FLT_MAX;
            
            for (auto& HitResult : OutHits)
            {
                float Distance = FVector::Dist(TraceStart, HitResult.Location);
                if (Distance < ClosestDistance)
                {
                    ClosestDistance = Distance;
                    ClosestHit = &HitResult;
                }
            }
            
            if (ClosestHit)
            {
                Wheels[i].bIsGrounded = true;
                Wheels[i].HitPoint = ClosestHit->Location;
                Wheels[i].HitNormal = ClosestHit->Normal;
                Wheels[i].SurfaceType = DetectSurfaceType(*ClosestHit);
            
                // Calculate suspension compression
                float Distance = FVector::Dist(TraceStart, ClosestHit->Location);
                float CompressionDistance = FMath::Max(0.0f, Wheels[i].SuspensionLength - (Distance - Wheels[i].WheelRadius));
                Wheels[i].SuspensionCompression = FMath::Clamp(CompressionDistance / Wheels[i].SuspensionLength, 0.0f, 1.0f);
            
                // Get wheel velocity
                FVector WheelVelocity = VehicleRoot->GetPhysicsLinearVelocityAtPoint(WheelWorldPos);
                CalculateWheelSlip(i, WheelCoords, WheelVelocity, DeltaTime);

                FVector FinalWheelLocation = FVector(WheelWorldPos.X, WheelWorldPos.Y, Wheels[i].HitPoint.Z)
                + FVector(0, 0, Wheels[i].SuspensionCompression * Wheels[i].SuspensionLength);

                // Update Wheel Transform
                UpdateWheelsMeshesLocation(i, FinalWheelLocation);
                UpdateWheelsMeshesRotation(i, WheelRight, DeltaTime, InputSystem);

                if (bIsInDebug && bDebugWheel)
                {
                    // Draw cylinder visualization
                    DrawCylinderDebug(FinalWheelLocation,WheelRight,
                        Wheels[i].WheelRadius, Wheels[i].WheelWidth);
                    DrawDebugSphere(GetWorld(), ClosestHit->Location, SmallSphereRadius, 8, FColor::Red);

                    if (bDebugWheelDirection)
                    {
                        // Draw wheel forward direction
                        DrawDebugLine(GetWorld(), WheelWorldPos, WheelWorldPos + WheelCoords.ForwardDir * 100.0f, 
                                     FColor::Purple, false, -1.0f, 0, 2.0f);
                    }
                }
            }
        }
        else
        {
            Wheels[i].bIsGrounded = false;
            Wheels[i].SuspensionCompression = 0.0f;
            Wheels[i].SlipRatio = 0.0f;
            Wheels[i].SlipAngle = 0.0f;
            Wheels[i].WheelLoad = 0.0f;
        }
    }
}

void UVehicleWheelSystemComponent::DrawCylinderDebug(const FVector& Center, const FVector& AxisDirection, float Radius, float HalfHeight) const
{
    if (!GetWorld()) return;
    
    int32 Segments = 32;
    FVector Up = FVector::UpVector;
    FVector Right = FVector::CrossProduct(AxisDirection, Up).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = FVector::RightVector;
    }
    Up = FVector::CrossProduct(Right, AxisDirection).GetSafeNormal();
    
    // Draw top circle
    FVector TopCenter = Center + AxisDirection * HalfHeight;
    for (int32 i = 0; i < Segments; i++)
    {
        float Angle1 = (2.0f * PI / Segments) * i;
        float Angle2 = (2.0f * PI / Segments) * ((i + 1) % Segments);
        
        FVector Point1 = TopCenter + (FMath::Cos(Angle1) * Right + FMath::Sin(Angle1) * Up) * Radius;
        FVector Point2 = TopCenter + (FMath::Cos(Angle2) * Right + FMath::Sin(Angle2) * Up) * Radius;
        
        DrawDebugLine(GetWorld(), Point1, Point2, FColor::Blue, false, -1.0f, 0, 2.0f);
    }
    
    // Draw bottom circle
    FVector BottomCenter = Center - AxisDirection * HalfHeight;
    for (int32 i = 0; i < Segments; i++)
    {
        float Angle1 = (2.0f * PI / Segments) * i;
        float Angle2 = (2.0f * PI / Segments) * ((i + 1) % Segments);
        
        FVector Point1 = BottomCenter + (FMath::Cos(Angle1) * Right + FMath::Sin(Angle1) * Up) * Radius;
        FVector Point2 = BottomCenter + (FMath::Cos(Angle2) * Right + FMath::Sin(Angle2) * Up) * Radius;
        
        DrawDebugLine(GetWorld(), Point1, Point2, FColor::Blue, false, -1.0f, 0, 2.0f);
    }
    
    // Draw connecting lines
    for (int32 i = 0; i < 8; i++)
    {
        float Angle = (2.0f * PI / 8) * i;
        FVector Offset = (FMath::Cos(Angle) * Right + FMath::Sin(Angle) * Up) * Radius;
        
        DrawDebugLine(GetWorld(), TopCenter + Offset, BottomCenter + Offset, FColor::Blue, false, -1.0f, 0, 2.0f);
    }
}

void UVehicleWheelSystemComponent::UpdateWheelsMeshesLocation(int WheelsIndex, const FVector& FinalWheelLocation) const
{
    if (VehicleOwner->GetWheelsMeshes().Num() != GetWheels().Num()) return;
    VehicleOwner->GetWheelsParent()[WheelsIndex]->SetWorldLocation(FinalWheelLocation);
}

void UVehicleWheelSystemComponent::UpdateWheelsMeshesRotation(int WheelsIndex, const FVector& WheelRight, float DeltaTime, UVehicleInputComponent* InputSystem)
{
    if (VehicleOwner->GetWheelsMeshes().Num() != GetWheels().Num()) return;

    FWheelData& Wheel = Wheels[WheelsIndex];
    UStaticMeshComponent* WheelMesh = VehicleOwner->GetWheelsMeshes()[WheelsIndex];
    USceneComponent* WheelMeshParent = VehicleOwner->GetWheelsParent()[WheelsIndex];
    
    // Get the current rotation
    FQuat CurrentQuat = WheelMesh->GetRelativeRotation().Quaternion();
    
    // Apply wheel spin
    float RotationDeltaRad = Wheel.WheelAngularVelocity * DeltaTime;
    FQuat SpinDeltaQuat = FQuat(WheelsIndex % 2 == 0 ? FVector::RightVector : -FVector::RightVector, RotationDeltaRad / PI);
    FQuat NewQuat = CurrentQuat * SpinDeltaQuat;

    // Apply Steer rotation
    FVector Up = VehicleOwner->GetActorForwardVector();
    FVector Right = FVector::CrossProduct(WheelRight, Up).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = VehicleOwner->GetActorRightVector();
    }
    Up = FVector::CrossProduct(Right, WheelRight).GetSafeNormal();

    
    WheelMesh->SetRelativeRotation(NewQuat);
    WheelMeshParent->SetWorldRotation(Up.ToOrientationRotator());
}


#pragma endregion
// ========================================

// ========================================
#pragma region Tire Forces

void UVehicleWheelSystemComponent::UpdateTireForces(float DeltaTime, UVehicleInputComponent* InputSystem)
{
    if (!VehicleRoot || !InputSystem) return;
    
    float SteerInput = InputSystem->GetSteering();
    
    for (int32 i = 0; i < Wheels.Num(); i++)
    {
        if (!Wheels[i].bIsGrounded) continue;
        
        // Calculate wheel coordinate system with steering
        FWheelCoordinateSystem WheelCoords = CalculateWheelCoordinateSystem(i, SteerInput);
        
        // Calculate tire forces
        FVector TireForce = CalculateTireForces(i, WheelCoords);
        
        // Apply friction clamps to prevent unrealistic forces
        ApplyFrictionClamps(i, WheelCoords, TireForce, DeltaTime);
        
        // Apply tire force
        FVector WheelWorldPos = GetWheelWorldPosition(i);
        if (!TireForce.IsZero() && !TireForce.ContainsNaN())
        {
            VehicleRoot->AddForceAtLocation(TireForce, WheelWorldPos);
        }
        
        Wheels[i].TireForce = TireForce;
    }
}

FVector UVehicleWheelSystemComponent::CalculateTireForces(int32 WheelIndex, 
    const FWheelCoordinateSystem& WheelCoords)
{
    if (!Wheels[WheelIndex].bIsGrounded) return FVector::ZeroVector;
    
    FTireSettings InTireSettings = GetTireSettingsForSurface(Wheels[WheelIndex].SurfaceType);
    float NormalLoad = Wheels[WheelIndex].WheelLoad;
    
    // Calculate grip forces using Pacjeka
    float LongitudinalGrip = CalculateLongitudinalGrip(WheelIndex, Wheels[WheelIndex].SlipRatio, NormalLoad, InTireSettings);
    float LateralGrip = CalculateLateralGrip(WheelIndex, Wheels[WheelIndex].SlipAngle, NormalLoad, InTireSettings);
    
    // Calculate tire forces in wheel coordinate system
    FVector LongitudinalForce = -WheelCoords.ForwardDir * LongitudinalGrip * FMath::Sign(Wheels[WheelIndex].SlipRatio);
    FVector LateralForce = -WheelCoords.RightDir * LateralGrip * FMath::Sign(Wheels[WheelIndex].SlipAngle);
    
    return LongitudinalForce + LateralForce;
}

void UVehicleWheelSystemComponent::CalculateWheelSlip(int32 WheelIndex, 
    const FWheelCoordinateSystem& WheelCoords, const FVector& WheelVelocity, float DeltaTime)
{
    if (!Wheels[WheelIndex].bIsGrounded) return;
    
    // Project wheel velocity onto wheel coordinate system
    float LongitudinalWheelSpeed = FVector::DotProduct(WheelVelocity, WheelCoords.ForwardDir);
    float LateralWheelSpeed = FVector::DotProduct(WheelVelocity, WheelCoords.RightDir);
    
    // Calculate slip ratio
    bool bWheelStopped = FMath::Abs(Wheels[WheelIndex].WheelAngularVelocity) < EPSILON;
    float SlideSign = bWheelStopped ? FMath::Sign(LongitudinalWheelSpeed) : 
                                     FMath::Sign(Wheels[WheelIndex].WheelAngularVelocity);
    
    float WheelSlipSpeed = (Wheels[WheelIndex].WheelAngularVelocity * Wheels[WheelIndex].WheelRadius) - 
                           LongitudinalWheelSpeed;
    WheelSlipSpeed *= SlideSign;
    
    FVector VehicleVelocity = VehicleRoot->GetPhysicsLinearVelocity();
    float VehicleSpeed = VehicleVelocity.Size();

    if (FMath::Abs(LongitudinalWheelSpeed) > EPSILON)
    {
        Wheels[WheelIndex].SlipRatio = (LongitudinalWheelSpeed - VehicleSpeed) / FMath::Abs(LongitudinalWheelSpeed);
    }
    else
    {
        Wheels[WheelIndex].SlipRatio = 0.0f;
    }
    
    // Calculate slip angle
    if (FMath::Abs(LongitudinalWheelSpeed) > EPSILON)
    {
        Wheels[WheelIndex].SlipAngle = FMath::Atan2(LateralWheelSpeed, FMath::Abs(LongitudinalWheelSpeed));
    }
    else
    {
        Wheels[WheelIndex].SlipAngle = 0.0f;
    }

    /*GEngine->AddOnScreenDebugMessage(-1, GetWorld()->DeltaTimeSeconds, FColor::White, 
                   FString::Printf(TEXT("Wheel[%d] - SlipRatio: %.2f | SlipAngle: %.2f°"),
                       WheelIndex, Wheels[WheelIndex].SlipRatio, 
                       FMath::RadiansToDegrees(Wheels[WheelIndex].SlipAngle)));*/
    
    // Update wheel angular velocity
    if (Wheels[WheelIndex].bIsGrounded)
    {
        float TargetAngularVel = LongitudinalWheelSpeed / Wheels[WheelIndex].WheelRadius;
        Wheels[WheelIndex].WheelAngularVelocity = FMath::FInterpTo(
            Wheels[WheelIndex].WheelAngularVelocity, TargetAngularVel, DeltaTime, 5.0f);
    }
}

float UVehicleWheelSystemComponent::CalculateLongitudinalGrip(int WheelIndex, float SlipRatio, float NormalLoad,
    const FTireSettings& TireConfig) const
{
    float AbsSlipRatio = FMath::Abs(SlipRatio);
    float GripScale = 1.0f;

    // Calculating Pacejka
    float SlipRatioRad = FMath::Abs(SlipRatio);
    float SlipRatioClamped = FMath::Clamp(SlipRatioRad, -2.0f, 2.0f);
    float B = TireConfig.B_Longitudinal; 
    float C = TireConfig.C_Longitudinal;
    float D = TireConfig.D_Longitudinal;
    float E = TireConfig.E_Longitudinal;
    GripScale = PacejkaFormula(SlipRatioClamped, B, C, D, E);
    GripScale = FMath::Clamp(GripScale, 0.0f, 1.0f);
    
    // Load sensitivity
    float LoadFactor = FMath::Pow(NormalLoad / (VehicleMass * GRAVITY / 4.0f), TireConfig.LoadSensitivity);

    float LongitudinalGrip = GripScale * TireConfig.PeakLongitudinalGrip * TireConfig.FrictionCoefficient * NormalLoad * LoadFactor;
    if (bIsInDebug && bDebugLongitudinal)
    {
        GEngine->AddOnScreenDebugMessage(-1, GetWorld()->DeltaTimeSeconds, FColor::Yellow, 
               FString::Printf(TEXT("Wheel: %d |LongitudinalGrip: %.1f | NormalizedSlip: %.1f | GripScale: %.1f"),
                   WheelIndex, LongitudinalGrip, SlipRatioClamped, GripScale));

        FLinearColor StartColor = FLinearColor::Green;
        FLinearColor EndColor = FLinearColor::Red;
        FLinearColor LerpColor = FLinearColor::LerpUsingHSV(StartColor, EndColor, GripScale);
        FColor DebugColor = LerpColor.ToFColor(true);
            
        DrawDebugSphere(GetWorld(), Wheels[WheelIndex].HitPoint + FVector(0.0f, 0.0f, Wheels[WheelIndex].WheelRadius), 
                       Wheels[WheelIndex].WheelRadius, 16, DebugColor);
    }
    return LongitudinalGrip;
}

float UVehicleWheelSystemComponent::CalculateLateralGrip(int WheelIndex, float SlipAngle, float NormalLoad,
    const FTireSettings& TireConfig) const
{
    float AbsSlipAngleDeg = FMath::Abs(FMath::RadiansToDegrees(SlipAngle));
    float GripScale = 1.0f;

    // Calculating Pacejka
    float SlipAngleRad = FMath::Abs(SlipAngle);
    float MaxSlipAngle = PI / 2.0f;
    float NormalizedSlip = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, MaxSlipAngle),
        FVector2D(0.0f, 1.0f),SlipAngleRad);
    
    float B = TireConfig.B_Lateral; 
    float C = TireConfig.C_Lateral;
    float D = 1.0f;
    float E = TireConfig.E_Lateral;

    GripScale = PacejkaFormula(NormalizedSlip, B, C, D, E);
    GripScale = FMath::Clamp(GripScale, 0.0f, 1.0f);
    
    // Load sensitivity
    float LoadFactor = FMath::Pow(NormalLoad / (VehicleMass * GRAVITY / 4.0f), TireConfig.LoadSensitivity);

    float LateralGrip = GripScale * TireConfig.PeakLateralGrip * TireConfig.FrictionCoefficient * NormalLoad * LoadFactor;
    if (bIsInDebug && bDebugLateral)
    {
        GEngine->AddOnScreenDebugMessage(-1, GetWorld()->DeltaTimeSeconds, FColor::Yellow, 
               FString::Printf(TEXT("Wheel: %d | SlipAngleRad: %.1f | NormalizedSlip: %.1f | GripScale: %.1f"),WheelIndex, SlipAngleRad, NormalizedSlip, GripScale));

        FLinearColor StartColor = FLinearColor::Green;
        FLinearColor EndColor = FLinearColor::Red;
        FLinearColor LerpColor = FLinearColor::LerpUsingHSV(StartColor, EndColor, GripScale);
        FColor DebugColor = LerpColor.ToFColor(true);

        FVector DebugArrowStart = Wheels[WheelIndex].HitPoint + FVector(0.0f, 0.0f, Wheels[WheelIndex].SuspensionLength);
        FVector DebugArrowEnd = DebugArrowStart + (Wheels[WheelIndex].TireForce.GetSafeNormal2D() * (90)) * GripScale;
            
        DrawDebugDirectionalArrow(GetWorld(), DebugArrowStart, DebugArrowEnd, 20.0f * GripScale, DebugColor
            ,false, -1.0f, 0, 5.0f);
            
        /*DrawDebugSphere(GetWorld(), Wheels[WheelIndex].HitPoint + FVector(0.0f, 0.0f, Wheels[WheelIndex].WheelRadius), 
                       Wheels[WheelIndex].WheelRadius, 16, DebugColor);*/
    }
    return LateralGrip;
}

void UVehicleWheelSystemComponent::ApplyFrictionClamps(int32 WheelIndex, 
    const FWheelCoordinateSystem& WheelCoords, FVector& TireForce, float DeltaTime)
{
    if (DeltaTime <= 0.0f) return;
    
    // Friction clamps prevent unrealistic forces that would flip velocity sign
    float WheelLoadFactor = Wheels[WheelIndex].WheelLoad / (VehicleMass * GRAVITY);
    float UsefulMass = WheelLoadFactor * VehicleMass;
    
    // Get wheel velocity
    FVector WheelWorldPos = GetWheelWorldPosition(WheelIndex);
    FVector WheelVelocity = VehicleRoot->GetPhysicsLinearVelocityAtPoint(WheelWorldPos);
    
    float LongitudinalVel = FVector::DotProduct(WheelVelocity, WheelCoords.ForwardDir);
    float LateralVel = FVector::DotProduct(WheelVelocity, WheelCoords.RightDir);
    
    // Clamp forces to prevent velocity sign flipping
    float MaxLongitudinalForce = UsefulMass * FMath::Abs(LongitudinalVel) / DeltaTime;
    float MaxLateralForce = UsefulMass * FMath::Abs(LateralVel) / DeltaTime;
    
    float LongitudinalForce = FVector::DotProduct(TireForce, WheelCoords.ForwardDir);
    float LateralForce = FVector::DotProduct(TireForce, WheelCoords.RightDir);
    
    // Apply clamps
    LongitudinalForce = FMath::Clamp(LongitudinalForce, -MaxLongitudinalForce, MaxLongitudinalForce);
    LateralForce = FMath::Clamp(LateralForce, -MaxLateralForce, MaxLateralForce);

    TireForce = WheelCoords.ForwardDir * LongitudinalForce + WheelCoords.RightDir * LateralForce;
    
    if (TireForce.Size() > MAX_WHEEL_FORCE)
    {
        TireForce = TireForce.GetSafeNormal() * MAX_WHEEL_FORCE;
    }
}

#pragma endregion
// ========================================

// ========================================
#pragma region Suspension
void UVehicleWheelSystemComponent::UpdateSuspension(float DeltaTime)
{
    if (!VehicleRoot) return;
    
    float WeightPerWheel = (VehicleMass * GRAVITY) / Wheels.Num();
    
    for (int32 i = 0; i < Wheels.Num(); i++)
    {
        if (!Wheels[i].bIsGrounded) continue;
        
        FSuspensionSettings SuspSettings = GetSuspensionSettingsForSurface(Wheels[i].SurfaceType);
        
        // Calculate spring force
        float RestPosition = SuspSettings.RestLength;
        float CompressionFromRest = Wheels[i].SuspensionCompression - RestPosition;
        float SpringForce = SuspSettings.SpringStrength * CompressionFromRest + WeightPerWheel;
        
        // Calculate damping force
        float CompressionVelocity = 0.0f;
        if (DeltaTime > 0.0f)
        {
            CompressionVelocity = (Wheels[i].SuspensionCompression - PreviousSuspensionCompressions[i]) / DeltaTime;
        }
        float DampingForce = CompressionVelocity * SuspSettings.DamperStrength;
        
        // Total suspension force
        float TotalSuspensionForce = SpringForce + DampingForce;
        TotalSuspensionForce = FMath::Clamp(TotalSuspensionForce, 0.0f, SuspSettings.MaxSuspensionForce);
        
        // Store wheel load for tire calculations
        Wheels[i].WheelLoad = TotalSuspensionForce;
        
        // Apply suspension force
        FVector SuspensionForce = Wheels[i].HitNormal * TotalSuspensionForce;
        FVector WheelWorldPos = GetWheelWorldPosition(i);

        if (bIsInDebug && bDebugSuspension)
        {
            DrawDebugLine(GetWorld(), WheelWorldPos, WheelWorldPos + SuspensionForce * 0.01f, 
                         FColor::Green, false, -1.0f, 0, 2.0f);
        }
        
        if (!SuspensionForce.ContainsNaN())
        {
            VehicleRoot->AddForceAtLocation(SuspensionForce, WheelWorldPos);
        }
        
        PreviousSuspensionCompressions[i] = Wheels[i].SuspensionCompression;
    }
}
#pragma endregion
// ========================================

// ========================================
#pragma region Braking
void UVehicleWheelSystemComponent::UpdateBraking(float DeltaTime, UVehicleInputComponent* InputSystem)
{
    if (!VehicleRoot || !InputSystem) return;
    
    float TotalBrakeInput = FMath::Max(InputSystem->GetBrake(), InputSystem->GetHandbrake() ? 1.0f : 0.0f);
    if (TotalBrakeInput <= 0.0f) return;
    
    // Get vehicle velocity once
    FVector VehicleVelocity = VehicleRoot->GetPhysicsLinearVelocity();
    float VehicleSpeed = VehicleVelocity.Size();
    
    // Dont apply force is vehicle is nearly stopped
    const float MinSpeedThreshold = 10.0f; // cm/s
    if (VehicleSpeed < MinSpeedThreshold)
    {
        // Just zero out wheel angular velocities when stopped
        for (int32 i = 0; i < Wheels.Num(); i++)
        {
            Wheels[i].WheelAngularVelocity = 0.0f;
        }
        return;
    }

    // TODO : fix cette partie pour avoir un frein qui applique moins de force du cote de la direction prise afin de ne pas faire un tete a queue
    float SteerInput = 0; // HOT Fix => Force zero to ensure the vehicule is not impacted by the direction when braking
    
    for (int32 i = 0; i < Wheels.Num(); i++)
    {
        if (!Wheels[i].bIsGrounded) continue;
        
        // Handbrake => rear wheels, regular => affect all
        float WheelBrakeInput = TotalBrakeInput;
        if (InputSystem->GetHandbrake() && IsSteerableWheel(i))
        {
            WheelBrakeInput = InputSystem->GetBrake();
        }
        
        // Get wheel velocity and coordinate system with steering
        FVector WheelWorldPos = GetWheelWorldPosition(i);
        FVector WheelVelocity = VehicleRoot->GetPhysicsLinearVelocityAtPoint(WheelWorldPos);
        FWheelCoordinateSystem WheelCoords = CalculateWheelCoordinateSystem(i, SteerInput);
        
        // Calculate longitudinal velocity
        float LongitudinalVel = FVector::DotProduct(WheelVelocity, WheelCoords.ForwardDir);
        
        if (FMath::Abs(LongitudinalVel) > EPSILON)
        {
            // Calculate desired brake force
            float BrakeForceAmount = BrakeForce * WheelBrakeInput;
            
            // Calculate the force needed to stop the wheel in this frame
            float WheelMass = VehicleRoot->GetMass() / Wheels.Num(); 
            float MaxStoppingForce = FMath::Abs(LongitudinalVel) * WheelMass / DeltaTime;
            
            // Clamp brake force to not exceed what's needed to stop
            BrakeForceAmount = FMath::Min(BrakeForceAmount, MaxStoppingForce);

            // Apply brake force opposite to current velocity direction
            FVector BrakeForceVector = -FMath::Sign(LongitudinalVel) * WheelCoords.ForwardDir * BrakeForceAmount;
            
            if (!BrakeForceVector.ContainsNaN())
            {
                VehicleRoot->AddForceAtLocation(BrakeForceVector, WheelWorldPos);
            }
            
            if (bIsInDebug && bDebugWheel && bDebugWheelDirection)
            {
                DrawDebugLine(GetWorld(), WheelWorldPos, WheelWorldPos + BrakeForceVector * 0.01f, 
                             FColor::Blue, false, -1.0f, 0, 2.0f);
                GEngine->AddOnScreenDebugMessage(-1, GetWorld()->DeltaTimeSeconds, FColor::Yellow, 
                    FString::Printf(TEXT("Velocity: %.1f, LongVel: %.1f"), VehicleSpeed, LongitudinalVel));
            }
        }
    }
}
#pragma endregion
// ========================================

// ========================================
#pragma region Surface detection
ECustomSurfaceType UVehicleWheelSystemComponent::DetectSurfaceType(const FHitResult& HitResult) const
{
    ECustomSurfaceType SurfaceType = ECustomSurfaceType::Road;
    
    // Check if we hit a surface with a physical mat
    if (HitResult.PhysMaterial.IsValid())
    {
        FString MaterialName = HitResult.PhysMaterial->GetName();
        
        if (MaterialName.Contains("PhysMat_Road"))
        {
            SurfaceType = ECustomSurfaceType::Road;
        }
            
        else if (MaterialName.Contains("PhysMat_Grass"))
        {
            SurfaceType = ECustomSurfaceType::Grass;
        }
            
    }

    if (bIsInDebug)
    {
        GEngine->AddOnScreenDebugMessage(-1, GetWorld()->DeltaTimeSeconds, FColor::Yellow, 
               FString::Printf(TEXT("Detected Surface: %s"), *UEnum::GetValueAsString(SurfaceType)));
    }
    
    return SurfaceType;
}

FTireSettings UVehicleWheelSystemComponent::GetTireSettingsForSurface(ECustomSurfaceType SurfaceType)
{
    if (TireSettings.Contains(SurfaceType))
    {
        return TireSettings[SurfaceType];
    }
    
    // Create default settings for missing surface types
    FTireSettings DefaultSettings;
    
    switch (SurfaceType)
    {
    case ECustomSurfaceType::Grass:
        DefaultSettings.PeakSlipRatio = 0.25f;
        DefaultSettings.PeakSlipAngle = 12.0f;
        DefaultSettings.PeakLongitudinalGrip = 0.6f;
        DefaultSettings.PeakLateralGrip = 0.7f;
        DefaultSettings.FrictionCoefficient = 0.5f;
        DefaultSettings.LoadSensitivity = 0.6f;
        TireSettings.Add(SurfaceType, DefaultSettings);
        break;
            
    case ECustomSurfaceType::Road:
    default:
        if (TireSettings.Contains(ECustomSurfaceType::Road))
        {
            DefaultSettings = TireSettings[ECustomSurfaceType::Road];
        }
        else
        {
            DefaultSettings.PeakSlipRatio = 0.15f;
            DefaultSettings.PeakSlipAngle = 8.0f;
            DefaultSettings.PeakLongitudinalGrip = 1.0f;
            DefaultSettings.PeakLateralGrip = 1.2f;
            DefaultSettings.FrictionCoefficient = 0.8f;
            DefaultSettings.LoadSensitivity = 0.8f;
            TireSettings.Add(ECustomSurfaceType::Road, DefaultSettings);
        }
        break;
    }

    // for the moment 
    return DefaultSettings;
}

FSuspensionSettings UVehicleWheelSystemComponent::GetSuspensionSettingsForSurface(ECustomSurfaceType SurfaceType)
{
    if (SuspensionSettings.Contains(SurfaceType))
    {
        return SuspensionSettings[SurfaceType];
    }
    
    // Create default settings for missing surface types
    FSuspensionSettings DefaultSettings;
    
    switch (SurfaceType)
    {
    case ECustomSurfaceType::Grass:
        DefaultSettings.SpringStrength = 40000.0f;
        DefaultSettings.DamperStrength = 3000.0f;
        DefaultSettings.MaxSuspensionForce = 55000.0f;
        DefaultSettings.RestLength = 0.75f;
        SuspensionSettings.Add(SurfaceType, DefaultSettings);
        break;
            
    case ECustomSurfaceType::Road:
    default:
        if (SuspensionSettings.Contains(ECustomSurfaceType::Road))
        {
            DefaultSettings = SuspensionSettings[ECustomSurfaceType::Road];
        }
        else
        {
            DefaultSettings.SpringStrength = 45000.0f;
            DefaultSettings.DamperStrength = 3500.0f;
            DefaultSettings.MaxSuspensionForce = 60000.0f;
            DefaultSettings.RestLength = 0.7f;
            SuspensionSettings.Add(ECustomSurfaceType::Road, DefaultSettings);
        }
        break;
    }
    
    return DefaultSettings;
}
#pragma endregion
// ========================================


// ========================================
// Helpers
// ========================================
TArray<FWheelData> UVehicleWheelSystemComponent::GetWheels() const
{
    return Wheels;
}

FWheelData UVehicleWheelSystemComponent::GetWheelData(int32 Index) const
{
    if (Wheels.IsValidIndex(Index))
    {
        return Wheels[Index];
    }
    return FWheelData();
}


float UVehicleWheelSystemComponent::PacejkaFormula(float Slip, float B, float C, float D, float E)
{
    float x = Slip;
    float term = B * x - E * (B * x - FMath::Atan(B * x));
    float y = D * FMath::Sin(C * FMath::Atan(term));

    return y;
}

