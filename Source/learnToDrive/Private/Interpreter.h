// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture.h"

#include "PreOpenCVHeaders.h"
#include "OpenCVHelper.h"

#include <ThirdParty/OpenCV/include/opencv2/imgproc.hpp>
#include <ThirdParty/OpenCV/include/opencv2/highgui/highgui.hpp>

#include <ThirdParty/OpenCV/include/opencv2/core.hpp>
#include "PostOpenCVHeaders.h"

#include "Interpreter.generated.h"

using namespace cv;

UCLASS()
class AInterpreter : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInterpreter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		UTextureRenderTarget2D* TextureRenderRef = nullptr;
	UPROPERTY(BlueprintReadOnly)
		UTexture2D* Texture1 = nullptr;

	UPROPERTY(BlueprintReadOnly)
		UTexture2D* Texture2 = nullptr;

	UPROPERTY(BlueprintReadOnly)
		UTexture2D* Texture3 = nullptr;

	bool flipflop;
		//FRenderTarget* RenderTarget = nullptr;

	// The rate at which the color data array and video texture is updated (in frames per second)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
		float RefreshRate;
	// The refresh timer
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
		float RefreshTimer;

	// The videos width and height (width, height)
	UPROPERTY(BlueprintReadWrite, Category = Camera)
		FVector2D VideoSize;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Data)
		TArray<FColor> ColorData;


	UPROPERTY(EditDefaultsOnly, Category = "Create Binary Image")
		FVector2D GradientThresh = FVector2D(60, 150);

	UPROPERTY(EditDefaultsOnly, Category = "Create Binary Image")
		FVector2D KSizeForBlur = FVector2D(6, 6);
	
	UPROPERTY(EditDefaultsOnly, Category = "Create Binary Image")
		FVector2D SChannelThresh = FVector2D(80, 255);

	UPROPERTY(EditDefaultsOnly, Category = "Create Binary Image")
		FVector2D LChannelThresh = FVector2D(120, 240);

	UPROPERTY(EditDefaultsOnly, Category = "Create Binary Image")
		FVector2D  BChannelThresh = FVector2D(80, 255);

	UPROPERTY(EditDefaultsOnly, Category = "Create Binary Image")
		FVector2D  AChannelThresh = FVector2D(110, 255);

	UPROPERTY(EditDefaultsOnly, Category = "Create Binary Image")
		FVector2D L2ChannelThresh = FVector2D(255, 255);

	uint8 LUTb[256];
	uint8 LUTl[256];
	uint8 LUTs[256];
	uint8 LUTa[256];

public:
	//functions
	cv::Mat da;
	// reads the current video frame
	UFUNCTION(BlueprintCallable, Category = Data)
		void ReadFrame();

	void GetHLS(Mat inputRGB, Mat& outputH, Mat& outputL, Mat& outputS);

	void GetLAB(Mat inputRGB, Mat& outputL, Mat& outputA, Mat& outputB);

	Mat BinaryThresholdLAB_LUV(Mat inputRGB, const FVector2D bThreshold, const FVector2D lThreshold);

	Mat GradientThreshold(Mat inputRGB, int channel, const FVector2D threshold);
	
	void CreateLUT(uint8* LUT, FVector2D Threshold);


};
