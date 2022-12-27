// Fill out your copyright notice in the Description page of Project Settings.


#include "ImageProcessor.h"
#include "Saver.h"
#include "Kismet/GameplayStatics.h"


// Sets default values for this component's properties
UImageProcessor::UImageProcessor()
{
	PrimaryComponentTick.bCanEverTick = false;

	refToLookUpTables.Add(&RGB_Thresholds.BinaryThresholds.ThresholdFirstChanel);
	refToLookUpTables.Add(&RGB_Thresholds.BinaryThresholds.ThresholdSecondChanel);
	refToLookUpTables.Add(&RGB_Thresholds.BinaryThresholds.ThresholdThirdChanel);
	refToLookUpTables.Add(&RGB_Thresholds.SobelThresholds.ThresholdFirstChanel);
	refToLookUpTables.Add(&RGB_Thresholds.SobelThresholds.ThresholdSecondChanel);
	refToLookUpTables.Add(&RGB_Thresholds.SobelThresholds.ThresholdThirdChanel);
	refToLookUpTables.Add(&LAB_Thresholds.BinaryThresholds.ThresholdFirstChanel);
	refToLookUpTables.Add(&LAB_Thresholds.BinaryThresholds.ThresholdSecondChanel);
	refToLookUpTables.Add(&LAB_Thresholds.BinaryThresholds.ThresholdThirdChanel);
	refToLookUpTables.Add(&LAB_Thresholds.SobelThresholds.ThresholdFirstChanel);
	refToLookUpTables.Add(&LAB_Thresholds.SobelThresholds.ThresholdSecondChanel);
	refToLookUpTables.Add(&LAB_Thresholds.SobelThresholds.ThresholdThirdChanel);
	refToLookUpTables.Add(&HLS_Thresholds.BinaryThresholds.ThresholdFirstChanel);
	refToLookUpTables.Add(&HLS_Thresholds.BinaryThresholds.ThresholdSecondChanel);
	refToLookUpTables.Add(&HLS_Thresholds.BinaryThresholds.ThresholdThirdChanel);
	refToLookUpTables.Add(&HLS_Thresholds.SobelThresholds.ThresholdFirstChanel);
	refToLookUpTables.Add(&HLS_Thresholds.SobelThresholds.ThresholdSecondChanel);
	refToLookUpTables.Add(&HLS_Thresholds.SobelThresholds.ThresholdThirdChanel);
	//LoadSettingsClear();
}


// Called when the game starts
void UImageProcessor::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("Da"));
	//create LUTs
	GenerateLookUpTables();
}

cv::Mat UImageProcessor::ConvertImage(cv::Mat inputImage, int code)
{
	cv::Mat Output;
	cv::cvtColor(inputImage, Output, code);
	return Output;
}

void BreakImage(cv::Mat inputImage, OUT cv::Mat& firstChanel, OUT cv::Mat& secondChanel, OUT cv::Mat& thirdChanel)
{
	cv::Mat channel[3];
	cv::split(inputImage, channel);
	firstChanel = channel[0];
	secondChanel = channel[1];
	thirdChanel = channel[2];
}

void UImageProcessor::CreateLUT(uint8* LUT, FVector2D Threshold)
{
	for (uint16 i = 0; i < 256; i++)
	{
		if (i >= Threshold[0] && i <= Threshold[1])
		{
			LUT[i] = 1;
		}
		else
		{
			LUT[i] = 0;
		}
	}
}

void UImageProcessor::GenerateLookUpTables()
{
	for(FChanelThreshold* iter : refToLookUpTables)
	{
		if (iter->UseThreshold)
		{
			CreateLUT(iter->LookupTable, iter->Threshold);
		}
	}
}

void UImageProcessor::SaveData()
{
	UE_LOG(LogTemp, Warning, TEXT("Saved"));
	USaver* SaveGameInstance = Cast<USaver>(UGameplayStatics::CreateSaveGameObject(USaver::StaticClass()));
	//add data to be saved 
	for (FChanelThreshold* iter : refToLookUpTables)
	{
		SaveGameInstance->addToSaveData(iter->Threshold, iter->UseThreshold);
	}

	//save data to slot
	UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("Thresholds"), 0);
}

void UImageProcessor::LoadData()
{
	UE_LOG(LogTemp, Warning, TEXT("Loaded"));
	USaver* SaveGameInstance = Cast<USaver>(UGameplayStatics::CreateSaveGameObject(USaver::StaticClass()));
	SaveGameInstance = Cast<USaver>(UGameplayStatics::LoadGameFromSlot("Thresholds",0));
	int8 index = 0;
	if (SaveGameInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("no save found"));
		return;
	}
	for (FChanelThreshold* iter : refToLookUpTables)
	{		
		iter->Threshold = SaveGameInstance->ThresholdsToBeSaved[index].Threshold;
		iter->UseThreshold = SaveGameInstance->ThresholdsToBeSaved[index].UseThreshold;
		index++;
	}
}

void UImageProcessor::LoadSettingsClear()
{
	USaver* SaveGameInstance = Cast<USaver>(UGameplayStatics::CreateSaveGameObject(USaver::StaticClass()));
	SaveGameInstance = Cast<USaver>(UGameplayStatics::LoadGameFromSlot("Thresholds", 0));
	if (SaveGameInstance == nullptr)
	{
		return;
	}
	int8 index = 0;
	for (FChanelThreshold* iter : refToLookUpTables)
	{
		iter->Threshold = SaveGameInstance->ThresholdsToBeSaved[index].Threshold;
		iter->UseThreshold = SaveGameInstance->ThresholdsToBeSaved[index].UseThreshold;
		index++;
	}
}
