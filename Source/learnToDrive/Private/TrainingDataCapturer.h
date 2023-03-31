// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"

#include "TrainingDataCapturer.generated.h"


UCLASS()
class UTrainingDataCapturer : public USceneCaptureComponent2D
{
	GENERATED_BODY()

public:

	UTrainingDataCapturer();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)override;


	int8 PersonalId = 0;

	class AVehiclePawn* Parent = nullptr;

	int8 TickingFreq = 1;


protected:
	virtual void BeginPlay() override;
	UPROPERTY(EditDefaultsOnly)
		int32 VideoWidth = 512;
	UPROPERTY(EditDefaultsOnly)
		int32 VideoHeight = 128;

private:
	UPROPERTY()
		UTextureRenderTarget2D* RenderTarget = nullptr;
	UPROPERTY()
		FString ImageFilePath;
	UPROPERTY()
		FString extension;

	EImageFormat ImageFormat;

	TSharedPtr<IImageWrapper> ImageWrapper = nullptr;

	FString CsvFilePath;
	float DT;
	int32 ImageId = 0;

	bool WriteRowToCSV(const FString& FilePath, const TArray<FString>& Row);

	bool SaveCameraViewToDisk(const FString& FilePath);

	void SaveTrainingData();

};
