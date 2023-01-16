
#include "Interpreter.h"
//#include "Components/InputComponent.h"
#include "ImageProcessor.h"

AInterpreter::AInterpreter()
{
	PrimaryActorTick.bCanEverTick = true;
	VideoSize = FVector2D(512, 128);
	RefreshRate = 30.0f;

	ImageProcesingUnit = CreateDefaultSubobject<UImageProcessor>("Image Procesor");

}

// Called when the game starts or when spawned
void AInterpreter::BeginPlay()
{
	Super::BeginPlay();
	BindInput();
	CreateTextures();

	ColorData.AddDefaulted(VideoSize.X * VideoSize.Y);
	flipflop = true;
	RefreshTimer = 0.f;

	CreateLUT(LUTb, BChannelThresh);
	CreateLUT(LUTs, SChannelThresh);
	CreateLUT(LUTl, LChannelThresh);
	CreateLUT(LUTa, AChannelThresh);

	srcPts[0] = Point2f(SourcePoints[0].X, SourcePoints[0].Y);
	srcPts[1] = Point2f(SourcePoints[1].X, SourcePoints[1].Y);
	srcPts[2] = Point2f(SourcePoints[2].X, SourcePoints[2].Y);
	srcPts[3] = Point2f(SourcePoints[3].X, SourcePoints[3].Y);

	destPts[0] = Point2f(Offset, 0);
	destPts[1] = Point2f(Offset, VideoSize.Y);
	destPts[2] = Point2f(VideoSize.X - Offset, VideoSize.Y);
	destPts[3] = Point2f(VideoSize.X - Offset, 0);
}
//stay
void AInterpreter::CreateTextures()
{
	if (TextureRenderRef)
	{
		Texture1 = TextureRenderRef->ConstructTexture2D(this, TEXT("Camera Image Raw"), EObjectFlags::RF_NoFlags, CTF_DeferCompression);
		Texture2 = TextureRenderRef->ConstructTexture2D(this, TEXT("Camera Image Edges"), EObjectFlags::RF_NoFlags, CTF_DeferCompression);
		Texture3 = TextureRenderRef->ConstructTexture2D(this, TEXT("Camera Image combo"), EObjectFlags::RF_NoFlags, CTF_DeferCompression);
	}
}
//stay
void AInterpreter::BindInput()
{
	InputComponent = NewObject<UInputComponent>(this);
	InputComponent->RegisterComponent();
	if (InputComponent)
	{
		InputComponent->BindAction("Save", IE_Pressed, this, &AInterpreter::Save);
		InputComponent->BindAction("Load", IE_Pressed, this, &AInterpreter::Load);
		EnableInput(GetWorld()->GetFirstPlayerController());
	}
}

//Stay
void AInterpreter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RefreshTimer += DeltaTime;

	// Read the pixels from the RenderTarget and store them in a FColor array

	if (TextureRenderRef)
	{
		if (Texture2)
		{
			if (RefreshTimer >= 1.0f / RefreshRate) 
			{
				//TextureRenderRef->UpdateTexture2D(Texture1, TextureRenderRef->GetTextureFormatForConversionToTexture2D(), EConstructTextureFlags::CTF_DeferCompression);
				ReadFrame();
				if (Texture1->PlatformData->Mips[0].BulkData.IsLocked() == false)
				{
					Texture1->UpdateResource();
				}
				RefreshTimer -= 1.0f / RefreshRate;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("renderTarget invalid"));
	}
}

void AInterpreter::ReadFrame()
{
	FRenderTarget* renderTarget = TextureRenderRef->GameThread_GetRenderTargetResource();
	renderTarget->ReadPixels(ColorData);

	Mat colorData = Mat(Size(512,128), CV_8UC4, ColorData.GetData());

	Mat perspective;
	if (PerspectiveWarpBinary || PerspectiveWarpRaw)
	{
		perspective = getPerspectiveTransform(srcPts, destPts);
		if (PerspectiveWarpBinary)
		{
			warpPerspective(colorData, colorData, perspective, Size(VideoSize.X, VideoSize.Y));
		}
	}
	Mat sBinary = 
		ImageProcesingUnit->PrelucrateImage(colorData);
	//BinaryThresholdLAB_LUV(colorData, BChannelThresh, LChannelThresh);

	Mat h, l, s;
	GetHLS(colorData, h, l, s);
	Mat sxBinary = GradientThreshold(s, 2, GradientThresh);
	
	Mat finalImage = Mat(Size(512, 128), CV_8UC4);
	for (uint16 i = 0; i < finalImage.cols; i++)
	{
		for (uint16 j = 0; j < finalImage.rows; j++)
		{
			uint8 result = 0;
			if (LabThreshold)
			{
				if (SobelThreshold)
				{
					result = ((uint8)(sBinary.at<uint8>(j, i)||sxBinary.at<uint8>(j, i)));
				}
				else
				{
					result = sBinary.at<uint8>(j, i);
				}
			}
			else
			{
				if (SobelThreshold)
				{
					result = sxBinary.at<uint8>(j, i);
				}
			}

			finalImage.at<Vec<uint8, 4>>(j, i)[0] = result * 255;
			finalImage.at<Vec<uint8, 4>>(j, i)[1] = result * 255;
			finalImage.at<Vec<uint8, 4>>(j, i)[2] = result * 255;
			finalImage.at<Vec<uint8, 4>>(j, i)[3] = 255;

			if (result == 1 && DrawEdgesInRed)
			{
				colorData.at<Vec<uint8, 4>>(j, i)[0] = 10;
				colorData.at<Vec<uint8, 4>>(j, i)[1] = 10;
				colorData.at<Vec<uint8, 4>>(j, i)[2] = 255;
			}

		}
	}
	
	if (DrawPerspectiveLines)
	{
		CreateAndDrawPerspectiveLines(colorData);
	}	
	if (PerspectiveWarpRaw && !PerspectiveWarpBinary)
	{
		warpPerspective(colorData, colorData, perspective, Size(VideoSize.X, VideoSize.Y));
	}
	

	
	Mat hist;
	Mat channel[4];

	if (Dilate)
	{
		dilate(finalImage, finalImage, Mat());
	}
	if (Blur)
	{
		blur(finalImage, finalImage, Size(6, 6));
	}

	split(finalImage, channel);
	reduce(channel[0], hist, 0, REDUCE_SUM, CV_32SC1);

	if (ShowHistogram)
	{
		Mat img = DrawHistogram(hist);
		colorData = img;
	}
	if (ShowMaxLane)
	{
		Point2i left, right;
		GetHistogramPeaksFinalMethod(hist, left, right);
		line(finalImage, Point(left.x, 0), Point(left.x, finalImage.rows), Scalar(255, 0, 0, 255), 2);
		line(finalImage, Point(right.x, 0), Point(right.x, finalImage.rows), Scalar(255, 0, 0, 255), 2);
	}

	CreateTextures(finalImage, colorData);
}

//stay
void AInterpreter::CreateTextures(cv::Mat& secondImage, cv::Mat& thirdImage)
{
	void* textureData = Texture2->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	const uint32 dataSize = 512 * 128 * 4 * sizeof(uint8);
	FMemory::Memcpy(textureData, secondImage.data, dataSize);

	Texture2->PlatformData->Mips[0].BulkData.Unlock();
	Texture2->UpdateResource();

	void* texture3Data = Texture3->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(texture3Data, thirdImage.data, dataSize);

	Texture3->PlatformData->Mips[0].BulkData.Unlock();
	Texture3->UpdateResource();
}
//stay for now
void AInterpreter::CreateAndDrawPerspectiveLines(cv::Mat& colorData)
{
	line(colorData, Point(srcPts[0]), Point(srcPts[1]), Scalar(255, 0, 0, 255), 2);
	line(colorData, Point(srcPts[1]), Point(srcPts[2]), Scalar(255, 0, 0, 255), 2);
	line(colorData, Point(srcPts[2]), Point(srcPts[3]), Scalar(255, 0, 0, 255), 2);
	line(colorData, Point(srcPts[3]), Point(srcPts[0]), Scalar(255, 0, 0, 255), 2);
}
//not yet
Mat AInterpreter::DrawHistogram(Mat& hist)
{
	uint16 hist_w = 512;
	uint16 hist_h = 128;
	uint16 histSize = 256;
	uint16 bin_w = cvRound((double)hist_w / histSize);

	Mat histImage(hist_h,hist_w,CV_8UC4,Scalar(0,0,0));
	//normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	for (int16 i = 1; i < hist_w; i++)
	{
		line(histImage,
			Point((i - 1), hist_h - hist.at<int32>(i - 1) /histSize),
			Point(i, hist_h - hist.at<int32>(i) /histSize),
			Scalar(255, 255, 255, 255),2);
	}
	Point2i left, right;
	if (UseBetterMethod)
	{
		GetHistogramPeaksFinalMethod(hist, left, right);
	}
	else
	{
		GetHistogramPeaksFirstMethod(hist, left, right);
	}
	left.y = hist_h - left.y / 256;
	right.y = hist_h - right.y / 256;
	circle(histImage, left, 3, Scalar(0, 0, 255, 255), 2);
	circle(histImage, right, 3, Scalar(0, 0, 255, 255), 2);

	return histImage;
}
//moved
void AInterpreter::GetHLS(Mat inputRGB, Mat& outputH, Mat& outputL, Mat& outputS)
{
	Mat Output; 
	cvtColor(inputRGB, Output, cv::COLOR_BGR2HLS);
	Mat channel[3];
	split(Output, channel);
	outputH = channel[0];
	outputL = channel[1];
	outputS = channel[2];
}
//moved
void AInterpreter::GetLAB(Mat inputRGB, Mat& outputL, Mat& outputA, Mat& outputB)
{
	Mat Output;
	cvtColor(inputRGB, Output, cv::COLOR_RGB2Lab);
	Mat channel[3];
	split(Output, channel);
	outputL = channel[0];
	outputA = channel[1];
	outputB = channel[2];
}

//binary threshold 
Mat AInterpreter::BinaryThresholdLAB_LUV(Mat inputRGB, const FVector2D bThreshold, const FVector2D lThreshold)
{
	//creates binary mask 
	Mat binaryImage = Mat(inputRGB.size(),CV_8UC1);
	Mat outL, outA, outB;
	GetLAB(inputRGB, outL, outA, outB);

	
	for (int16 i = 0; i < inputRGB.cols; i++)
	{
		for (int16 j = 0; j < inputRGB.rows; j++)
		{
			binaryImage.at<uint8>(j, i) = ((uint8)(LUTb[outB.at<uint8>(j, i)] == 1 && LUTl[outL.at<uint8>(j, i)] == 1 && LUTa[outA.at<uint8>(j, i)] == 1));
		}
	}
				
	return binaryImage;
}
//sobel threshold to be moved and rewrited 
cv::Mat AInterpreter::GradientThreshold(Mat input, int channel, const FVector2D threshold)
{
	Mat binaryImage = Mat(input.size(), CV_8UC1);
	Mat sobel;
	//blur
	blur(input, input, Size(KSizeForBlur.X,KSizeForBlur.Y));
	Sobel(input, sobel, CV_8U, 1, 0);
	
	for (uint16 i = 0; i < input.cols; i++)
	{
		for (uint16 j = 0; j < input.rows; j++)
		{
			binaryImage.at<uint8>(j, i) = ((uint8)(sobel.at<int8>(j, i) > threshold[0] && sobel.at<uint8>(j, i) < threshold[1]));
		}
	}
	return binaryImage;
}


//moved
void AInterpreter::CreateLUT(uint8* LUT, FVector2D Threshold)
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
//not yet
void AInterpreter::GetHistogramPeaksFirstMethod(Mat& hist, Point2i& leftMax, Point2i& rightMax)
{
	leftMax.y = rightMax.y = 0;
	for (uint16 i = 0; i < hist.cols/2; i++)
	{
		if (hist.at<int32>(i) > leftMax.y)
		{
			leftMax.x = i;
			leftMax.y = hist.at<int32>(i);
		}
	}
	for (uint16 i = hist.cols / 2; i < hist.cols; i++)
	{
		if (hist.at<int32>(i) > rightMax.y)
		{
			rightMax.x = i;
			rightMax.y = hist.at<int32>(i);
		}
	}
}
//not yet
void AInterpreter::GetHistogramPeaksFinalMethod(Mat& hist, Point2i& leftMax, Point2i& rightMax)
{

	Point2i maxLocal(0,0);
	for (uint16 i = 0; i < hist.cols; i++)
	{
		if (hist.at<int32>(i) > maxLocal.y)
		{
			maxLocal.x = i;
			maxLocal.y = hist.at<int32>(i);
		}
		if (maxLocal.y != 0)
		{
			if (hist.at<int32>(i) < 5*256)
			{
				if (maxLocal.y >= leftMax.y)
				{
					rightMax = leftMax;
					leftMax = maxLocal;
				}
				else if (maxLocal.y >= rightMax.y)
				{
					rightMax = maxLocal;
				}
				maxLocal = Point2i(0, 0);
			}
		}
	}
	if (maxLocal.y >= leftMax.y)
	{
		rightMax = leftMax;
		leftMax = maxLocal;
	}
	else if (maxLocal.y >= rightMax.y)
	{
		rightMax = maxLocal;
	}
	
}
//stay
void AInterpreter::Save()
{
	//dummy function 
	ImageProcesingUnit->SaveData();
}
//stay
void AInterpreter::Load()
{
	ImageProcesingUnit->LoadData();
}

//stay
void AInterpreter::GetThresholds(int32 index, FVector2D& threshold, bool& useThreshold)
{
	if (ImageProcesingUnit)
	{
		ImageProcesingUnit->GetThresholds(index, threshold, useThreshold);
	}
}
//stay
void AInterpreter::SetThresholds(int32 index, FVector2D threshold, bool useThreshold)
{
	if (ImageProcesingUnit)
	{
		ImageProcesingUnit->SetThresholds(index, threshold, useThreshold);
	}
}