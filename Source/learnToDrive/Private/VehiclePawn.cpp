// Fill out your copyright notice in the Description page of Project Settings.


#include "VehiclePawn.h"
#include "DrawDebugHelpers.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include <Kismet/GameplayStatics.h>

AVehiclePawn::AVehiclePawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	FrontPoint = CreateDefaultSubobject<USceneComponent>("FrontPoint");
	BackPoint = CreateDefaultSubobject<USceneComponent>("BackPoint");
	AdvancePoint = CreateDefaultSubobject<USceneComponent>("AdvancePoint");

	FrontPoint->SetupAttachment(RootComponent);
	BackPoint->SetupAttachment(RootComponent);
	AdvancePoint->SetupAttachment(RootComponent);
}

void AVehiclePawn::BeginPlay()
{
	Super::BeginPlay();

	ChaosWheeledVehicleComponent = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
	if (ChaosWheeledVehicleComponent == nullptr)
	{
		UE_LOG(LogTemp, Fatal, TEXT("Invalid cast to UChaosWheeledVehicleMovementComponent in VehiclePawn.cpp"));
	}
	BreakLights(false);

	//cruise controll
	PrevSpeedError = 30.f;
	Kp = 0.5;
	Kd = 0.01;

	Road = Cast<ARoad>(UGameplayStatics::GetActorOfClass(this, ARoad::StaticClass()));
	if (Road == nullptr)
	{
		UE_LOG(LogTemp, Fatal, TEXT("Failed to find actor of class ARoad"));
	}

	//steering
	FrontPoint->SetRelativeLocation(FVector(130, 0, 0));
	BackPoint->SetRelativeLocation(FVector(-125, 0, 0));
	AdvancePoint->SetRelativeLocation(FVector(700, 0, 0));
}

void AVehiclePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (ChaosWheeledVehicleComponent->IsMovingOnGround())
	{
		GetMesh()->SetAngularDamping(0);
	}
	else
	{
		GetMesh()->SetAngularDamping(3);
	}
 
	KeepRoad();
	CruiseControll(DeltaTime);
}

void AVehiclePawn::CruiseControll(float DeltaTime)
{
	//current speed
	float KPH = GetVehicleMovement()->GetForwardSpeed() * 0.036;
	//error
	float errorKPH = DesiredSpeed - KPH;

	//derivate
	float speedErrorDer = (errorKPH - PrevSpeedError) / DeltaTime;
	PrevSpeedError = errorKPH;


	float value = Kp * errorKPH + Kd * speedErrorDer;

	if (value > 1)
	{
		value = 1;
	}
	if (value < -1)
	{
		value = -1;
	}

	
	MoveForward(value);
}

void AVehiclePawn::KeepRoad()
{
	
	FVector advancePointCoordinates = AdvancePoint->GetComponentLocation();
	FVector frontPointCoordinates = FrontPoint->GetComponentLocation();
	FVector backPointCoordinates = BackPoint->GetComponentLocation();

	FVector coordOnSpline = Road->SplineComp->FindLocationClosestToWorldLocation(advancePointCoordinates,ESplineCoordinateSpace::World);
	coordOnSpline.Z = advancePointCoordinates.Z;
	//DrawDebugLine(GetWorld(), frontPointCoordinates, coordOnSpline, FColor::Red, false, -1,0,10);

	FVector L = frontPointCoordinates - backPointCoordinates;
	FVector ld = coordOnSpline - backPointCoordinates;
	float a = L.HeadingAngle() - ld.HeadingAngle();
	
	float LDist = FVector::Dist(backPointCoordinates,frontPointCoordinates);
	float angle = (atan((sin(a)*LDist * 2)/ld.Size()));

	ChaosWheeledVehicleComponent->SetSteeringInput(-angle*2);

	//set desired speed in order to be able to take coreners
	float curentSpeed = GetVehicleMovement()->GetForwardSpeed() * 0.036;
	if (FMath::Abs(angle) * 2 >= CriticalAngle)
	{
		float update = MaxSpeed - (MaxSpeed - 10) * FMath::Abs(angle) * 2;
		if (curentSpeed - update >= 20)
			DesiredSpeed = update;
	}
	else
		DesiredSpeed = MaxSpeed;

}



void AVehiclePawn::MoveForward(float value)
{
	if (value >= 0)
	{
		ChaosWheeledVehicleComponent->SetThrottleInput(value);
		ChaosWheeledVehicleComponent->SetBrakeInput(0);
		//turn off break lights
		if (BreakLightsState == true)
		{
			BreakLightsState = false;
			BreakLights(false);
		}
	}
	else
	{
		ChaosWheeledVehicleComponent->SetBrakeInput(value * -1);
		ChaosWheeledVehicleComponent->SetThrottleInput(0);
		//turn on break lights
		if (BreakLightsState == false)
		{
			BreakLightsState = true;
			BreakLights(true);
		}
	}
}

void AVehiclePawn::Steer(float value)
{
	ChaosWheeledVehicleComponent->SetSteeringInput(value);
}

void AVehiclePawn::HandBreak()
{
	ChaosWheeledVehicleComponent->SetHandbrakeInput(true);
}

void AVehiclePawn::HandBreakReleased()
{
	ChaosWheeledVehicleComponent->SetHandbrakeInput(false);
}

void AVehiclePawn::ResetCar()
{
	FVector desiredLocation = GetActorLocation();
	desiredLocation.Z += 50;
	FRotator desiredRotation = FRotator::ZeroRotator;
	desiredRotation.Yaw = GetActorRotation().Yaw;

	SetActorLocation(desiredLocation, true);
	SetActorRotation(desiredRotation);
	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
}


