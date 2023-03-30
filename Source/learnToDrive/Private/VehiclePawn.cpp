// Fill out your copyright notice in the Description page of Project Settings.


#include "VehiclePawn.h"
#include "DrawDebugHelpers.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include <Kismet/GameplayStatics.h>

AVehiclePawn::AVehiclePawn()
{
	// setup components
	PrimaryActorTick.bCanEverTick = true;
	FrontPoint = CreateDefaultSubobject<USceneComponent>("FrontPoint");
	BackPoint = CreateDefaultSubobject<USceneComponent>("BackPoint");
	AdvancePoint = CreateDefaultSubobject<USceneComponent>("AdvancePoint");
	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>("CameraComponent");

	FrontPoint->SetupAttachment(RootComponent);
	BackPoint->SetupAttachment(RootComponent);
	AdvancePoint->SetupAttachment(RootComponent);
	SceneCaptureComponent->SetupAttachment(RootComponent);

	SceneCaptureComponent->SetRelativeLocation(FVector(142, 0, 100));
	SceneCaptureComponent->SetRelativeRotation(FRotator(0, -10, 0));
	SceneCaptureComponent->FOVAngle = 120;
	SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCaptureComponent->bCaptureEveryFrame = false;

	ImageFilePath = FPaths::ProjectSavedDir() / TEXT("ScreenShots/CameraView");
	extension = TEXT("png");
	ImageFormat = EImageFormat::PNG;
	CsvFilePath = FPaths::ProjectSavedDir() / TEXT("Data.csv");
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
	AdvancePoint->SetRelativeLocation(FVector(400, 0, 0));

	RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->InitCustomFormat(VideoWidth, VideoHeight, PF_B8G8R8A8, false);

	SceneCaptureComponent->TextureTarget = RenderTarget;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
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
	SaveTrainingData();
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

	//get closest point on spline
	FVector coordOnSpline = Road->SplineComp->FindLocationClosestToWorldLocation(advancePointCoordinates,ESplineCoordinateSpace::World);
	coordOnSpline.Z = advancePointCoordinates.Z;

	if (DrawLine)
	{
		DrawDebugLine(GetWorld(), frontPointCoordinates, coordOnSpline, FColor::Red, false, -1,0,10);
	}

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
	if (ChaosWheeledVehicleComponent->GetHandbrakeInput())
		return;
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

bool AVehiclePawn::WriteRowToCSV(const FString& FilePath, const TArray<FString>& Row)
{
	FString RowLine;
	for (const FString& Cell : Row)
	{
		RowLine.Append(Cell + ",");
	}
	// Remove the last comma and add a newline character
	FString dumy = RowLine.LeftChop(1);
	RowLine.Append("\n");

	// Write the row to the file
	return FFileHelper::SaveStringToFile(RowLine, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}

bool AVehiclePawn::SaveCameraViewToDisk(const FString& FilePath)
{
	if (!SceneCaptureComponent || VideoWidth <= 0 || VideoHeight <= 0 || FilePath.IsEmpty())
	{
		return false;
	}
	// Create a render target texture with the desired dimensions
	SceneCaptureComponent->CaptureScene();

	// Read the captured data and create an image
	TArray<FColor> Bitmap;
	if (!RenderTarget)
	{
		return false;
	}
	FRenderTarget* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource->ReadPixels(Bitmap))
	{
		return false;
	}

	// Compress the image
	
	TArray<uint8> CompressedData;
	if (ImageWrapper->SetRaw(Bitmap.GetData(), Bitmap.GetAllocatedSize(), RenderTargetResource->GetSizeXY().X, RenderTargetResource->GetSizeXY().Y, ERGBFormat::BGRA, 8))
	{
		CompressedData = ImageWrapper->GetCompressed();
	}
	else
	{
		return false;
	}

	// Save the image to disk
	return FFileHelper::SaveArrayToFile(CompressedData, *FilePath);
}

void AVehiclePawn::SaveTrainingData()
{
	//save image to disk 	
	FString photoPath = FString::Printf(TEXT("%s_%d%d.%s"), *ImageFilePath, PersonalId, ImageId++, *extension);

	bool bSaved = SaveCameraViewToDisk(photoPath);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save camera view to %s"), *photoPath);
	}

	//save data to csv 
	// Generate data and write it to the file

	TArray<FString> DataRow = {
		photoPath,
		FString::SanitizeFloat(ChaosWheeledVehicleComponent->GetSteeringInput()),
		FString::SanitizeFloat(ChaosWheeledVehicleComponent->GetThrottleInput()),
		FString::SanitizeFloat(ChaosWheeledVehicleComponent->GetBrakeInput()),
		FString::SanitizeFloat(ChaosWheeledVehicleComponent->GetForwardSpeed())
	};
	if (!WriteRowToCSV(CsvFilePath, DataRow))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to write data row to file %s"), *CsvFilePath);
	}
}


