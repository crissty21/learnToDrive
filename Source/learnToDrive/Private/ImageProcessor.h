// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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
		TArray<int8> LookupTable;
};

USTRUCT(BlueprintType)
struct FThresholds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		FChanelThreshold ThresholdSobelFirstChanel;
	UPROPERTY(EditAnywhere)
		FChanelThreshold ThresholdSobelSecondChanel;
	UPROPERTY(EditAnywhere)
		FChanelThreshold ThresholdSobelThirdChanel;

	UPROPERTY(EditAnywhere)
		FChanelThreshold ThresholdBinaryFirstChanel;
	UPROPERTY(EditAnywhere)
		FChanelThreshold ThresholdBinarySecondChanel;
	UPROPERTY(EditAnywhere)
		FChanelThreshold ThresholdBinaryThirdChanel;
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

	/*
	Mat ConvertImage(Mat inputImage, int code);

	void BreakImage(Mat inputImage, OUT Mat& firstChanel, OUT Mat& SecondChanel, OUT Mat& ThirdChanel);
	/*
	{
		//breaks the hls color space in its specific components 
		Mat Output;
		
		cvtColor(inputRGB, Output, cv::COLOR_BGR2HLS);
		Mat channel[3];
		split(Output, channel);
		outputH = channel[0];
		outputL = channel[1];
		outputS = channel[2];
	}
	*/
};
