// Fill out your copyright notice in the Description page of Project Settings.


#include "ImageProcessor.h"

// Sets default values for this component's properties
UImageProcessor::UImageProcessor()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;


	// ...
}


// Called when the game starts
void UImageProcessor::BeginPlay()
{
	Super::BeginPlay();

	//create LUTs
}
/*
Mat UImageProcessor::ConvertImage(Mat inputImage, int code)
{
	Mat Output;
	cvtColor(inputRGB, Output, cv::COLOR_BGR2HLS);
	return Output;
}
*/

