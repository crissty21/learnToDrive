// Fill out your copyright notice in the Description page of Project Settings.

#include "Brain.h"
#include "VehiclePawn.h"
#include "TrainingDataCapturer.h"

UTrainingDataCapturer::UTrainingDataCapturer()
{
    PrimaryComponentTick.bCanEverTick = true;

    ImageFilePath = FPaths::ProjectSavedDir() / TEXT("ScreenShots/CameraView");
    extension = TEXT("jpeg");
}

void UTrainingDataCapturer::BeginPlay()
{
    Super::BeginPlay();

	gameMode = (ABrain*)GetWorld()->GetAuthGameMode();
	if (gameMode == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to cast gamemode to ABrain"));
		VideoHeight = 1;
		VideoWidth = 1;
	}
	else
	{
		VideoHeight = gameMode->VideoHeight;
		VideoWidth = gameMode->VideoWidth;
	}

    RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->InitCustomFormat(VideoWidth, VideoHeight, PF_B8G8R8A8, false);
    TextureTarget = RenderTarget;

}
void UTrainingDataCapturer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    SendTrainingData();
}

void UTrainingDataCapturer::SendTrainingData()
{
	//save image to disk 	
	FString photoPath = FString::Printf(TEXT("%s_%d%d.%s"), *ImageFilePath, PersonalId, ImageId++, *extension);

	// Read the captured data and create an image
	CaptureScene();
	TArray<FColor> bitmap;

	//read pixels
	FRenderTarget* renderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!renderTargetResource->ReadPixels(bitmap))
	{
		return;
	}
	gameMode->AddImageToSave(photoPath, bitmap);
	
	//save data to csv 
	// Generate data 
	if (Parent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Parent"));
		return;
	}
	
	TArray<FString> dataRow = {
		photoPath,
		FString::SanitizeFloat(Parent->GetSteering()),
		FString::SanitizeFloat(Parent->GetThrottle()),
		FString::SanitizeFloat(Parent->GetBreak()),
		FString::SanitizeFloat(Parent->GetSpeed())
	};
	gameMode->AddDataToSave(dataRow);
}


