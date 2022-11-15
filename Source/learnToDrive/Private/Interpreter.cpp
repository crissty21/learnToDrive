#include "Interpreter.h"

AInterpreter::AInterpreter()
{
	PrimaryActorTick.bCanEverTick = true;
	VideoSize = FVector2D(256, 256);
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
		Texture = TextureRenderRef->ConstructTexture2D(this, TEXT("Camera Image"), EObjectFlags::RF_NoFlags, CTF_DeferCompression);
	}
	flipflop = true;
	RefreshTimer = 0.f;
	// setup openCV 

	//cvSize = cv::Size(VideoSize.X, VideoSize.Y);
	//cvMat = new cv::Mat(cvSize, CV_8UC4, ColorData.GetData());

}

// Called every frame
void AInterpreter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RefreshTimer += DeltaTime;

	// Read the pixels from the RenderTarget and store them in a FColor array


	if (TextureRenderRef)
	{
		if (Texture)
		{
			if (RefreshTimer >= 1.0f / RefreshRate) 
			{
				UE_LOG(LogTemp, Warning, TEXT("%f %f"), RefreshTimer, 1.0f / RefreshRate);
				TextureRenderRef->UpdateTexture2D(Texture, TextureRenderRef->GetTextureFormatForConversionToTexture2D(), EConstructTextureFlags::CTF_DeferCompression);
				if (Texture->PlatformData->Mips[0].BulkData.IsLocked() == false)
				{
					Texture->UpdateResource();
				}
				RefreshTimer -= 1.0f / RefreshRate;
				//ReadFrame();
				//i might actually be a fucking genius 
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
	//Texture->PlatformData->Mips[0].BulkData.Unlock();

	//cv::Mat* colorData = new cv::Mat(cvSize, CV_8UC4, ColorData.GetData());

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
	FMemory::Memcpy(TextureData, Pixels, sizeof(uint8) * VideoSize.X * VideoSize.Y * 4);
	Mip->BulkData.Unlock();

	*/
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

