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

	//cvSize = cv::Size(VideoSize.X, VideoSize.Y);
	//cvMat = new cv::Mat(cvSize, CV_8UC4, ColorData.GetData());
	CreateLUT(LUTb, BChannelThresh);
	CreateLUT(LUTs, SChannelThresh);
	CreateLUT(LUTl, LChannelThresh);
	CreateLUT(LUTa, AChannelThresh);
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
	//FColor* FormatedImageData = static_cast<FColor*>(Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));


	FRenderTarget* renderTarget = TextureRenderRef->GameThread_GetRenderTargetResource();
	renderTarget->ReadPixels(ColorData);

	//Mat colorData = Mat(Size(512, 128), CV_8UC4, FormatedImageData);


	Mat colorData = Mat(Size(512,128), CV_8UC4, ColorData.GetData());

	Mat sBinary = BinaryThresholdLAB_LUV(colorData, BChannelThresh, LChannelThresh);
	Mat h, l, s;
	GetHLS(colorData, h, l, s);
	Mat sxBinary = GradientThreshold(s, 2, GradientThresh);
	
	Mat finalImage = Mat(Size(512, 128), CV_8UC4);
	
	for (int i = 0; i < finalImage.cols; i++)
	{
		for (int j = 0; j < finalImage.rows; j++)
		{
			uint8 result = ((uint8)(
				sBinary.at<uint8>(j, i)
				||
				sxBinary.at<uint8>(j, i)
				));

			finalImage.at<Vec<uint8, 4>>(j, i)[0] = result * 255;
			finalImage.at<Vec<uint8, 4>>(j, i)[1] = result * 255;
			finalImage.at<Vec<uint8, 4>>(j, i)[2] = result * 255;
			finalImage.at<Vec<uint8, 4>>(j, i)[3] = 255;

			if (result == 1)
			{
				colorData.at<Vec<uint8, 4>>(j, i)[0] = 10;
				colorData.at<Vec<uint8, 4>>(j, i)[1] = 10;
				colorData.at<Vec<uint8, 4>>(j, i)[2] = 255;
			}

		}
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

	{
		/*
		Vec3b data = outH.at<Vec3b>(0, 0);
		UE_LOG(LogTemp, Warning, TEXT("%i"), data[0]);
		*/
		//colorData->convertTo(*colorData, -1, 1, 0);

		//const int32 TextureDataSize = colorData->size().width * colorData->size().height * 4 * sizeof(uint8);

		// Lock the texture so we can read / write to it
		// set the texture data

		//FTexture2DMipMap* MyMipMap = &Texture->PlatformData->Mips[0];
		//FByteBulkData* RawImageData = &MyMipMap->BulkData;
		/*
		uint8* Pixels = new uint8[VideoSize.X * VideoSize.Y * 4];
		for (int32 y = 0; y < VideoSize.X; y++)
		{
			for (int32 x = 0; x < VideoSize.Y; x++)
			{
				int32 curPixelIndex = ((y * VideoSize.Y) + x);
				Pixels[4 * curPixelIndex] = ColorData[4 * curPixelIndex].B;
				Pixels[4 * curPixelIndex + 1] = ColorData[4 * curPixelIndex].G;
				Pixels[4 * curPixelIndex + 2] = ColorData[4 * curPixelIndex].R;
				Pixels[4 * curPixelIndex + 3] = ColorData[4 * curPixelIndex].A;
			}
		}
		// Allocate first mipmap.
		FTexture2DMipMap* Mip = new(Texture->PlatformData->Mips) FTexture2DMipMap();
		Mip->SizeX = VideoSize.X;
		Mip->SizeY = VideoSize.Y;

		Mip->BulkData.Lock(LOCK_READ_WRITE);
		uint8* TextureData = (uint8*)Mip->BulkData.Realloc(VideoSize.X * VideoSize.Y * 4);
		FMemory::Memcpy(TextureData, finalImage.data, sizeof(uint8) * VideoSize.X * VideoSize.Y * 4);
		Mip->BulkData.Unlock();

		
		/*
		//FColor* FormatedImageData = static_cast<FColor*>(RawImageData->Lock(LOCK_READ_WRITE));

		FColor* FormatedImageData = static_cast<FColor*>(Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
		uint32 TextureWidth = MyMipMap->SizeX, TextureHeight = MyMipMap->SizeY;
		for (uint32 x = 0; x < TextureWidth; x++)
		{
			for (uint32 y = 0; y < TextureHeight; y++)
			{
				FormatedImageData[y * TextureWidth + x] = ColorData[y * TextureWidth + x];
			}
		}
		RawImageData->Unlock();

		FormatedImageData = static_cast<FColor*>(Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_ONLY));
		UE_LOG(LogTemp, Warning, TEXT("%sDA"), *(FormatedImageData[0].ToString()));
		RawImageData->Unlock();

		//Texture->UpdateResource();
		//FMemory::Memcpy(TextureData, cvMat.data, TextureDataSize);
		//FMemory::ParallelMemcpy(TextureData, colorData->data, TextureDataSize);

		// Unlock the texture
		//PlayerControllerRef->Texture->PlatformData->Mips[0].BulkData.Unlock();
		// Apply Texture changes to GPU memory
		/*
		PlayerControllerRef->Texture->UpdateResource();
		Camera_Texture2D = PlayerControllerRef->Texture;
		PlayerControllerRef->UpdateTexture();
		//PlayerControllerRef->Texture = Camera_Texture2D;
		//pass this to MyHUD
		/*
		AMyHUD* hud = Cast<AMyHUD>(UGameplayStatics::GetPlayerController(this, 0)->GetHUD());
		if (hud)
		{
			hud->DrawTexture(texture, 10, 10, 256, 256, 1, 1, 256, 256);
		}*/

		//how to change texture to a image on a hud
		//update brush via texture on a image in a widget class

	}
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

void AInterpreter::GetLUV(Mat inputRGB, Mat& outputL, Mat& outputU, Mat& outputV)
{
	Mat Output;
	cvtColor(inputRGB, Output, cv::COLOR_BGR2Luv);
	Mat channel[3];
	split(Output, channel);
	outputL = channel[0];
	outputU = channel[1];
	outputV = channel[2];
}

cv::Mat AInterpreter::BinaryThresholdLAB_LUV(Mat inputRGB, const FVector2D bThreshold, const FVector2D lThreshold)
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

cv::Mat AInterpreter::BinaryThresholdHLS(Mat inputRGB, const FVector2D sThreshold, const FVector2D lThreshold)
{
	Mat binaryImage = Mat(inputRGB.size(), CV_8UC1);
	Mat outH, outL, outS;
	GetHLS(inputRGB, outH, outL, outS);
	for (int i = 0; i < inputRGB.cols; i++)
	{
		for (int j = 0; j < inputRGB.rows; j++)
		{
			binaryImage.at<uint8>(j, i) = ((uint8)(LUTs[outS.at<uint8>(j, i)] && LUTl[outL.at<uint8>(j, i)]));
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
	/*for (int i = 0; i < sobel.rows; i++)
	{
		for (int j = 0; j < sobel.cols; j++)
		{
			sobel.at<uint8>(i,j) = FMath::Abs(sobel.at<int8>(i, j));
		}
	}*/
	
	for (int i = 0; i < input.cols; i++)
	{
		for (int j = 0; j < input.rows; j++)
		{
			uint8& pixel = binaryImage.at<uint8>(j, i) = ((uint8)(sobel.at<int8>(j, i) > threshold[0] && sobel.at<uint8>(j, i) < threshold[1]));
		}
	}
	//binaryImage = sobel;
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

