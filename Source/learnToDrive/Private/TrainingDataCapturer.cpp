// Fill out your copyright notice in the Description page of Project Settings.

#include "VehiclePawn.h"
#include "TrainingDataCapturer.h"

UTrainingDataCapturer::UTrainingDataCapturer()
{

    // Enable tick and set the tick interval
    PrimaryComponentTick.bCanEverTick = true;


    ImageFilePath = FPaths::ProjectSavedDir() / TEXT("ScreenShots/CameraView");
    extension = TEXT("jpeg");
    ImageFormat = EImageFormat::JPEG;
    CsvFilePath = FPaths::ProjectSavedDir() / TEXT("Data.csv");


}
void UTrainingDataCapturer::BeginPlay()
{
    Super::BeginPlay();

    RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->InitCustomFormat(VideoWidth, VideoHeight, PF_B8G8R8A8, false);

    TextureTarget = RenderTarget;

    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

	gameMode = (ABrain*)GetWorld()->GetAuthGameMode();
	if (gameMode == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to cast gamemode to ABrain"));
	}

}
void UTrainingDataCapturer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Check if the game is running
    SaveTrainingData();
	DT = DeltaTime;
}

bool UTrainingDataCapturer::WriteRowToCSV(const FString& FilePath, const TArray<FString>& Row)
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

bool UTrainingDataCapturer::SaveCameraViewToDisk(const FString& FilePath)
{
	if (VideoWidth <= 0 || VideoHeight <= 0 || FilePath.IsEmpty())
	{
		return false;
	}

	// Create a render target texture with the desired dimensions
	CaptureScene();

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
	gameMode->AddImageToSave(FilePath, Bitmap);
	/*
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
	return FFileHelper::SaveArrayToFile(CompressedData, *FilePath);*/
	return true;
}

void UTrainingDataCapturer::SaveTrainingData()
{
	//save image to disk 	
	FString photoPath = FString::Printf(TEXT("%s_%d%d.%s"), *ImageFilePath, PersonalId, ImageId++, *extension);

	bool bSaved = SaveCameraViewToDisk(photoPath);
	if (!true)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save camera view to %s"), *photoPath);
	}

	//save data to csv 
	// Generate data and write it to the file
	if (Parent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Parent"), *photoPath);
		return;
	}
	
	TArray<FString> DataRow = {
		photoPath,
		FString::SanitizeFloat(Parent->GetSteering()),
		FString::SanitizeFloat(Parent->GetThrottle()),
		FString::SanitizeFloat(Parent->GetBreak()),
		FString::SanitizeFloat(Parent->GetSpeed()),
		FString::SanitizeFloat(DT)
	};
	gameMode->AddDataToSave(DataRow);
	/*
	if (!WriteRowToCSV(CsvFilePath, DataRow))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to write data row to file %s"), *CsvFilePath);
	}*/
}


