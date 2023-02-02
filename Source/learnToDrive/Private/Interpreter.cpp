
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
	//flipflop = true;
	RefreshTimer = 0.f;

	srcPts[0] = cv::Point2f(SourcePoints[0].X, SourcePoints[0].Y);
	srcPts[1] = cv::Point2f(SourcePoints[1].X, SourcePoints[1].Y);
	srcPts[2] = cv::Point2f(SourcePoints[2].X, SourcePoints[2].Y);
	srcPts[3] = cv::Point2f(SourcePoints[3].X, SourcePoints[3].Y);

	destPts[0] = cv::Point2f(Offset, 0);
	destPts[1] = cv::Point2f(Offset, VideoSize.Y);
	destPts[2] = cv::Point2f(VideoSize.X - Offset, VideoSize.Y);
	destPts[3] = cv::Point2f(VideoSize.X - Offset, 0);
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

	cv::Mat colorData = cv::Mat(cv::Size(512,128), CV_8UC4, ColorData.GetData());

	cv::Mat perspective;
	if (PerspectiveWarpBinary || PerspectiveWarpRaw)
	{
		perspective = getPerspectiveTransform(srcPts, destPts);
		if (PerspectiveWarpBinary)
		{
			warpPerspective(colorData, colorData, perspective, cv::Size(VideoSize.X, VideoSize.Y));
		}
	}

	cv::Mat finalImage = ImageProcesingUnit->PrelucrateImage(colorData);
	//we get a single channel image, so we need to convert it to a 4 channel image to be displayed on screen 
	cv::cvtColor(finalImage, finalImage, cv::COLOR_GRAY2RGBA);

	if (DrawPerspectiveLines)
	{
		CreateAndDrawPerspectiveLines(colorData);
	}	
	if (PerspectiveWarpRaw && !PerspectiveWarpBinary)
	{
		warpPerspective(colorData, colorData, perspective, cv::Size(VideoSize.X, VideoSize.Y));
	}
	

	
	cv::Mat hist;
	cv::Mat channel[4];

	split(finalImage, channel);
	reduce(channel[0], hist, 0, cv::REDUCE_SUM, CV_32SC1);

	if (ShowHistogram)
	{
		cv::Mat img = DrawHistogram(hist);
		colorData = img;
	}
	if (ShowMaxLane)
	{
		cv::Point2i left, right;
		GetHistogramPeaksFinalMethod(hist, left, right);
		line(finalImage, cv::Point(left.x, 0), cv::Point(left.x, finalImage.rows), cv::Scalar(255, 0, 0, 255), 2);
		line(finalImage, cv::Point(right.x, 0), cv::Point(right.x, finalImage.rows), cv::Scalar(255, 0, 0, 255), 2);
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
	line(colorData, cv::Point(srcPts[0]), cv::Point(srcPts[1]), cv::Scalar(255, 0, 0, 255), 2);
	line(colorData, cv::Point(srcPts[1]), cv::Point(srcPts[2]), cv::Scalar(255, 0, 0, 255), 2);
	line(colorData, cv::Point(srcPts[2]), cv::Point(srcPts[3]), cv::Scalar(255, 0, 0, 255), 2);
	line(colorData, cv::Point(srcPts[3]), cv::Point(srcPts[0]), cv::Scalar(255, 0, 0, 255), 2);
}
//not yet
cv::Mat AInterpreter::DrawHistogram(cv::Mat& hist)
{
	uint16 hist_w = 512;
	uint16 hist_h = 128;
	uint16 histSize = 256;
	uint16 bin_w = cvRound((double)hist_w / histSize);

	cv::Mat histImage(hist_h,hist_w,CV_8UC4, cv::Scalar(0,0,0));
	//normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	for (int16 i = 1; i < hist_w; i++)
	{
		line(histImage,
			cv::Point((i - 1), hist_h - hist.at<int32>(i - 1) /histSize),
			cv::Point(i, hist_h - hist.at<int32>(i) /histSize),
			cv::Scalar(255, 255, 255, 255),2);
	}
	cv::Point2i left, right;
	GetHistogramPeaksFinalMethod(hist, left, right);

	left.y = hist_h - left.y / 256;
	right.y = hist_h - right.y / 256;
	circle(histImage, left, 3, cv::Scalar(0, 0, 255, 255), 2);
	circle(histImage, right, 3, cv::Scalar(0, 0, 255, 255), 2);

	return histImage;
}
//not yet
void AInterpreter::GetHistogramPeaksFinalMethod(cv::Mat& hist, cv::Point2i& leftMax, cv::Point2i& rightMax)
{

	cv::Point2i maxLocal(0,0);
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
				maxLocal = cv::Point2i(0, 0);
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