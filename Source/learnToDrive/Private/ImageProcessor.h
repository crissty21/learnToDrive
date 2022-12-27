// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "PreOpenCVHeaders.h"
#include "OpenCVHelper.h"

#include <ThirdParty/OpenCV/include/opencv2/imgproc.hpp>
#include <ThirdParty/OpenCV/include/opencv2/highgui/highgui.hpp>

#include <ThirdParty/OpenCV/include/opencv2/core.hpp>
#include "PostOpenCVHeaders.h"

#include "ImageProcessor.generated.h"


USTRUCT(BlueprintType)
struct FChanelThreshold
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere)
		FVector2D Threshold = FVector2D(0);
	UPROPERTY(EditAnywhere)
		bool UseThreshold = false;
	UPROPERTY()
		uint8 LookupTable[256];
};

USTRUCT(BlueprintType)
struct FSpaceTheshold
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		FChanelThreshold ThresholdFirstChanel;
	UPROPERTY(EditAnywhere)
		FChanelThreshold ThresholdSecondChanel;
	UPROPERTY(EditAnywhere)
		FChanelThreshold ThresholdThirdChanel;
};

USTRUCT(BlueprintType)
struct FThresholds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		FSpaceTheshold SobelThresholds;
	UPROPERTY(EditAnywhere)
		FSpaceTheshold BinaryThresholds;

};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UImageProcessor : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UImageProcessor();

protected:
	virtual void BeginPlay() override;

public:
	
	UPROPERTY(EditAnywhere)
		FThresholds RGB_Thresholds;
	UPROPERTY(EditAnywhere)
		FThresholds HLS_Thresholds;
	UPROPERTY(EditAnywhere)
		FThresholds LAB_Thresholds;
	
	cv::Mat ConvertImage(cv::Mat inputImage, int code);

	void BreakImage(cv::Mat inputImage, OUT cv::Mat& firstChanel, OUT cv::Mat& SecondChanel, OUT cv::Mat& ThirdChanel);

	void CreateLUT(uint8* LUT, FVector2D Threshold);

	void GenerateLookUpTables();

	void SaveData();
	void LoadData();
	void LoadSettingsClear();

private:
	TArray<FChanelThreshold*> refToLookUpTables;
};
