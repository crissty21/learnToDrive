// Fill out your copyright notice in the Description page of Project Settings.


#include "ImageProcessor.h"
#include "Saver.h"
#include "Kismet/GameplayStatics.h"


// Sets default values for this component's properties
UImageProcessor::UImageProcessor()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UImageProcessor::BeginPlay()
{
	Super::BeginPlay();
	refToLookUpTables.SetNum(18);
	//create LUTs
	LoadSettingsClear();
	GenerateLookUpTables();
}

cv::Mat UImageProcessor::ConvertImage(cv::Mat inputImage, int code)
{
	cv::Mat Output;
	cv::cvtColor(inputImage, Output, code);
	return Output;
}

void UImageProcessor::BreakImage(cv::Mat inputImage, OUT cv::Mat& firstChanel, OUT cv::Mat& secondChanel, OUT cv::Mat& thirdChanel)
{
	cv::Mat channel[3];
	cv::Mat breakk = cv::Mat(inputImage);
	cv::split(breakk, channel);
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
	for(FChanelThreshold& iter : refToLookUpTables)
	{
		if (iter.UseThreshold)
		{
			CreateLUT(iter.LookupTable, iter.Threshold);
		}
	}
}

void UImageProcessor::SaveData()
{
	UE_LOG(LogTemp, Warning, TEXT("Saved"));
	USaver* SaveGameInstance = Cast<USaver>(UGameplayStatics::CreateSaveGameObject(USaver::StaticClass()));
	//add data to be saved 
	for (FChanelThreshold& iter : refToLookUpTables)
	{
		SaveGameInstance->addToSaveData(iter.Threshold, iter.UseThreshold);
	}

	//save data to slot
	UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("Thresholds"), 0);
}

void UImageProcessor::LoadData()
{
	UE_LOG(LogTemp, Warning, TEXT("Loaded"));
	USaver* SaveGameInstance = Cast<USaver>(UGameplayStatics::LoadGameFromSlot("Thresholds",0));
	int8 index = 0;
	if (SaveGameInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("no save found"));
		return;
	}
	for (FChanelThreshold& iter : refToLookUpTables)
	{		
		iter.Threshold = SaveGameInstance->ThresholdsToBeSaved[index].Threshold;	
		iter.UseThreshold = SaveGameInstance->ThresholdsToBeSaved[index].UseThreshold;
		index++;
	}
}

void UImageProcessor::LoadSettingsClear()
{
	USaver* SaveGameInstance = Cast<USaver>(UGameplayStatics::LoadGameFromSlot("Thresholds", 0));
	if (SaveGameInstance == nullptr)
	{
		return;
	}
	int8 index = 0;
	for (FChanelThreshold& iter : refToLookUpTables)
	{
		iter.Threshold = SaveGameInstance->ThresholdsToBeSaved[index].Threshold;
		iter.UseThreshold = SaveGameInstance->ThresholdsToBeSaved[index].UseThreshold;
		index++;
	}
}

void UImageProcessor::SetThresholds(int32 index, FVector2D threshold, bool useThreshold)
{
	if (refToLookUpTables.IsValidIndex(index))
	{
		refToLookUpTables[index].Threshold = threshold;
		refToLookUpTables[index].UseThreshold = useThreshold;
	}
}

bool UImageProcessor::checkUsageBinary(const int8* table)
{
	return refToLookUpTables[table[0]].UseThreshold || refToLookUpTables[table[1]].UseThreshold || refToLookUpTables[table[2]].UseThreshold;
}
bool UImageProcessor::checkUsageSobel(const int8* table)
{
	return refToLookUpTables[table[0]].UseThreshold || refToLookUpTables[table[1]].UseThreshold || refToLookUpTables[table[2]].UseThreshold;
}
cv::Mat UImageProcessor::BinaryThreshold(cv::Mat input, const int8* threshold)
{
	cv::Mat binaryImage = cv::Mat(input.size(), CV_8UC1);

	//HERE


	//probabil nu sunt mapate cum trebuie variabileleai 

	for (int16 i = 0; i < input.cols; i++)
	{
		for (int16 j = 0; j < input.rows; j++)
		{
			bool pixel = true;
			if (refToLookUpTables[threshold[0]].UseThreshold)
			{
				pixel = pixel && (refToLookUpTables[threshold[0]].LookupTable[input.at<cv::Vec<uint8, 3>>(j, i)[0]] == 1);
			}
			if (pixel && refToLookUpTables[threshold[1]].UseThreshold)
			{
				pixel = pixel && (refToLookUpTables[threshold[1]].LookupTable[input.at<cv::Vec<uint8, 3>>(j, i)[1]] == 1);
			}
			if (pixel && refToLookUpTables[threshold[2]].UseThreshold)
			{
				pixel = pixel && (refToLookUpTables[threshold[2]].LookupTable[input.at<cv::Vec<uint8, 3>>(j, i)[2]] == 1);
			}
			binaryImage.at<uint8>(j, i) = ((uint8)(pixel));

		}
	}
	return binaryImage;
}

cv::Mat UImageProcessor::OrMats(const cv::Mat first, const cv::Mat second)
{
	cv::Mat binaryImage = cv::Mat(first.size(), CV_8UC1);

	for (int16 i = 0; i < first.cols; i++)
	{
		for (int16 j = 0; j < first.rows; j++)
		{
			binaryImage.at<uint8>(j, i) = ((uint8)(
				first.at<uint8>(j,i) == 1 || second.at<uint8>(j,i) == 1
				));
		}
	}
	return binaryImage;
}

cv::Mat UImageProcessor::PrelucrateImage(cv::Mat image)
{
	cv::Mat finalImage = cv::Mat(image.size(), CV_8UC1);
	cv::Mat result;
	bool added = false;
	//we check to see for each image type
	if (checkUsageBinary(RGBs))
	{
		result = BinaryThreshold(image,RGBs);
		if (added)
		{
			finalImage = OrMats(finalImage, result);
		}
		else
		{
			added = true;
			finalImage = result;
		}

	}
	if (checkUsageSobel(RGBs))
	{
		//do sobel thresholds for rgb
		//save the result with OR in final image
	}
	if (checkUsageBinary(LABs) || checkUsageSobel(LABs))
	{
		cv::Mat	labImage = ConvertImage(image, cv::COLOR_BGR2Lab);
		if (checkUsageBinary(LABs))
		{
			result = BinaryThreshold(labImage, LABs);
			if (added)
			{
				finalImage = OrMats(finalImage, result);
			}
			else
			{
				added = true;
				finalImage = result;
			}

		}
		if (checkUsageSobel(LABs))
		{
			//do sobel threshold for lab
			//save result with OR in final image
		}
	}
	if (checkUsageBinary(HLSs) || checkUsageSobel(HLSs))
	{
		cv::Mat	hlsImage = ConvertImage(image, cv::COLOR_BGR2HLS);
		if (checkUsageBinary(HLSs))
		{
			result = BinaryThreshold(hlsImage, HLSs);
			if (added)
			{
				finalImage = OrMats(finalImage, result);
			}
			else
			{
				added = true;
				finalImage = result;
			}

		}
		if (checkUsageSobel(HLSs))
		{
			//do sobel threshold for hls
			//save result with OR in final image
		}
	}

	return finalImage;
}

void UImageProcessor::GetThresholds(int32 index, FVector2D& threshold, bool& useThreshold)
{
	if (refToLookUpTables.IsValidIndex(index))
	{
		threshold = refToLookUpTables[index].Threshold;
		useThreshold = refToLookUpTables[index].UseThreshold;
	}
}
