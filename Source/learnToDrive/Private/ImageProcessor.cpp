// Fill out your copyright notice in the Description page of Project Settings.


#include "ImageProcessor.h"
#include "Saver.h"
#include "Kismet/GameplayStatics.h"

/*


*/

UImageProcessor::UImageProcessor()
{
	PrimaryComponentTick.bCanEverTick = false;
}

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
	for (FChanelThreshold& iter : refToLookUpTables)
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
	USaver* SaveGameInstance = Cast<USaver>(UGameplayStatics::LoadGameFromSlot("Thresholds", 0));
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
	cv::Mat binaryImage = cv::Mat::ones(input.size(), CV_8UC1) * 255;

	if (input.channels() == 3)
	{
		cv::Mat chanels[3];
		cv::split(input, chanels);
		for (uint8 i = 0; i < 3; ++i)
		{
			if (refToLookUpTables[threshold[i]].UseThreshold)
			{
				cv::inRange(chanels[i], refToLookUpTables[threshold[i]].Threshold.X, refToLookUpTables[threshold[i]].Threshold.Y, chanels[i]);
				cv::threshold(chanels[i], chanels[i], 0,255,cv::THRESH_BINARY);
				cv::bitwise_and(binaryImage, chanels[i], binaryImage);
			}
		}
	}
	else
	{
		cv::Mat chanels[4];
		cv::split(input, chanels);
		cv::bitwise_or(chanels[1], 1, binaryImage);
		for (uint8 i = 0; i < 3; ++i)
		{
			if (refToLookUpTables[threshold[i]].UseThreshold)
			{
				cv::inRange(chanels[i], refToLookUpTables[threshold[i]].Threshold.X, refToLookUpTables[threshold[i]].Threshold.Y, chanels[i]);
				cv::threshold(chanels[i], chanels[i], 0, 255, cv::THRESH_BINARY);
				cv::bitwise_and(binaryImage, chanels[i], binaryImage);
			}
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
				first.at<uint8>(j, i) == 1 || second.at<uint8>(j, i) == 1
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

		result = BinaryThreshold(image, RGBs);
		if (added)
		{
			cv::bitwise_or(finalImage, result, finalImage);
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
				cv::bitwise_or(finalImage, result, finalImage);
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
				cv::bitwise_or(finalImage, result, finalImage);
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
	if (UseBlur)
	{
		cv::medianBlur(finalImage, finalImage, KernelBlurSize);
	}
	if (UseErodeDilate)
	{
		cv::erode(finalImage, finalImage, elementErode);
		cv::dilate(finalImage, finalImage, elementDilate);
	}
	cv::threshold(finalImage, finalImage, 0, 255, cv::THRESH_BINARY);
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

void UImageProcessor::SetThresholds(int32 index, FVector2D threshold, bool useThreshold)
{
	if (refToLookUpTables.IsValidIndex(index))
	{
		refToLookUpTables[index].Threshold = threshold;
		refToLookUpTables[index].UseThreshold = useThreshold;
		if (useThreshold)
		{
			CreateLUT(refToLookUpTables[index].LookupTable, threshold);
		}
	}
}
/*
* rgb third: 130 - 255
* hls third: 80 - 255
*     second: 20 - 70
*	  first: 15 - 20
*/