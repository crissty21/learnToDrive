// Fill out your copyright notice in the Description page of Project Settings.


#include "Brain.h"

ABrain::ABrain()
{

	// Enable tick and set the tick interval
	PrimaryActorTick.bCanEverTick = true;
	ImageFormat = EImageFormat::JPEG;
	CsvFilePath = FPaths::ProjectSavedDir() / TEXT("Data.csv");
}

void ABrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SaveTrainingData();
}

void ABrain::BeginPlay()
{
	Super::BeginPlay();
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

}

void ABrain::AddImageToSave(FString path, TArray<FColor> data)
{
	photos.Enqueue(TPair<FString, TArray<FColor>>(path, data));
}

void ABrain::AddDataToSave(TArray<FString> Row)
{
	CSVdata.Enqueue(Row);
}

bool ABrain::WriteRowToCSV(const FString& FilePath, const TArray<FString>& Row)
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

bool ABrain::SaveCameraViewToDisk(const FString& FilePath, TArray<FColor> Bitmap)
{
	// Compress the image

	TArray<uint8> CompressedData;
	if (ImageWrapper->SetRaw(Bitmap.GetData(), Bitmap.GetAllocatedSize(), VideoWidth, VideoHeight, ERGBFormat::BGRA, 8))
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

void ABrain::SaveTrainingData()
{

	if (photos.IsEmpty() == false)
	{

		TPair<FString, TArray<FColor>>* currentPhoto = photos.Peek();
		/*bool bSaved = SaveCameraViewToDisk(currentPhoto->Key, currentPhoto->Value);
		if (!bSaved)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save camera view to %s"), *currentPhoto->Key);
		}*/
		photos.Pop();
	}


	//save data to csv 
	// Generate data and write it to the file
	if (CSVdata.IsEmpty() == false)
	{
		if (!WriteRowToCSV(CsvFilePath, *CSVdata.Peek()))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to write data row to file %s"), *CsvFilePath);
		}
		CSVdata.Pop();
	}
}
