#include "Interpreter.h"

AInterpreter::AInterpreter()
{
	PrimaryActorTick.bCanEverTick = true;
	VideoSize = FVector2D(512, 128);
	RefreshRate = 30.0f;
}

// Called when the game starts or when spawned
void AInterpreter::BeginPlay()
{
	Super::BeginPlay();

	// Prepare the color data array
	ColorData.AddDefaulted(VideoSize.X * VideoSize.Y);
	//create texture from texture render ref
	if (TextureRenderRef)
	{
		Texture1 = TextureRenderRef->ConstructTexture2D(this, TEXT("Camera Image Raw"), EObjectFlags::RF_NoFlags, CTF_DeferCompression);
		Texture2 = TextureRenderRef->ConstructTexture2D(this, TEXT("Camera Image Edges"), EObjectFlags::RF_NoFlags, CTF_DeferCompression);
		Texture3 = TextureRenderRef->ConstructTexture2D(this, TEXT("Camera Image combo"), EObjectFlags::RF_NoFlags, CTF_DeferCompression);
	}
	flipflop = true;
	RefreshTimer = 0.f;
	// setup openCV 

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

// Called every frame
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
				TextureRenderRef->UpdateTexture2D(Texture1, TextureRenderRef->GetTextureFormatForConversionToTexture2D(), EConstructTextureFlags::CTF_DeferCompression);
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

	Mat sBinary = BinaryThresholdLAB_LUV(colorData, BChannelThresh, LChannelThresh);
	Mat h, l, s;
	GetHLS(colorData, h, l, s);
	Mat sxBinary = GradientThreshold(s, 2, GradientThresh);
	
	Mat finalImage = Mat(Size(512, 128), CV_8UC4);
	for (int i = 0; i < finalImage.cols; i++)
	{
		for (int j = 0; j < finalImage.rows; j++)
		{
			uint8 result;
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
		line(colorData, Point(srcPts[0]), Point(srcPts[1]), Scalar(255, 0, 0, 255), 2);
		line(colorData, Point(srcPts[1]), Point(srcPts[2]), Scalar(255, 0, 0, 255), 2);
		line(colorData, Point(srcPts[2]), Point(srcPts[3]), Scalar(255, 0, 0, 255), 2);
		line(colorData, Point(srcPts[3]), Point(srcPts[0]), Scalar(255, 0, 0, 255), 2);
	}	
	if (PerspectiveWarpRaw && !PerspectiveWarpBinary)
	{
		warpPerspective(colorData, colorData, perspective, Size(VideoSize.X, VideoSize.Y));
	}
	
	//Mat perspectiveInv = getPerspectiveTransform(InputArray(destPts), InputArray(srcPts));

	
	Mat hist;
	Mat channel[4];
	split(finalImage, channel);
	reduce(channel[0], hist, 0, REDUCE_SUM, CV_32SC1);

	if (ShowHistogram)
	{
		Mat img = DrawHistogram(hist);
		colorData = img;
	}



	void* textureData = Texture2->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	const int32 dataSize = 512 * 128 *4* sizeof(uint8);
	FMemory::Memcpy(textureData, finalImage.data, dataSize);

	Texture2->PlatformData->Mips[0].BulkData.Unlock();
	Texture2->UpdateResource();

	void* texture3Data = Texture3->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(texture3Data, colorData.data, dataSize);

	Texture3->PlatformData->Mips[0].BulkData.Unlock();
	Texture3->UpdateResource();
}

Mat AInterpreter::DrawHistogram(Mat& hist)
{
	int hist_w = 512;
	int hist_h = 128;
	int histSize = 256;
	int bin_w = cvRound((double)hist_w / histSize);

	Mat histImage(hist_h,hist_w,CV_8UC4,Scalar(0,0,0));
	//normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	for (int i = 1; i < hist_w; i++)
	{
		
		cv::line(
			histImage,
			cv::Point((i - 1), hist_h - hist.at<int32>(i - 1) /histSize),
			cv::Point(i, hist_h - hist.at<int32>(i) /histSize),
			cv::Scalar(255, 255, 255, 255),2);

	}
	return histImage;
}
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


Mat AInterpreter::BinaryThresholdLAB_LUV(Mat inputRGB, const FVector2D bThreshold, const FVector2D lThreshold)
{
	//creates binary mask 
	Mat binaryImage = Mat(inputRGB.size(),CV_8UC1);
	Mat outL, outA, outB;
	GetLAB(inputRGB, outL, outA, outB);
	
	for (int i = 0; i < inputRGB.cols; i++)
	{
		for (int j = 0; j < inputRGB.rows; j++)
		{
			binaryImage.at<uint8>(j, i) = ((uint8)(LUTb[outB.at<uint8>(j, i)] == 1 && LUTl[outL.at<uint8>(j, i)] == 1 && LUTa[outA.at<uint8>(j, i)] == 1));
		}
	}
	return binaryImage;
}

cv::Mat AInterpreter::GradientThreshold(Mat input, int channel, const FVector2D threshold)
{
	Mat binaryImage = Mat(input.size(), CV_8UC1);
	Mat sobel;
	//blur
	blur(input, input, Size(KSizeForBlur.X,KSizeForBlur.Y));
	Sobel(input, sobel, CV_8U, 1, 0);
	
	for (int i = 0; i < input.cols; i++)
	{
		for (int j = 0; j < input.rows; j++)
		{
			uint8& pixel = binaryImage.at<uint8>(j, i) = ((uint8)(sobel.at<int8>(j, i) > threshold[0] && sobel.at<uint8>(j, i) < threshold[1]));
		}
	}
	return binaryImage;
}

void AInterpreter::CreateLUT(uint8* LUT, FVector2D Threshold)
{
	for (int i = 0; i < 256; i++)
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

