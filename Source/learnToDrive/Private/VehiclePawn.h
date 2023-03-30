// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "Road.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"

#include "VehiclePawn.generated.h"

/**
 * 
 */
UCLASS()
class AVehiclePawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVehiclePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
		void BreakLights(bool state);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void CruiseControll(float DeltaTime);
	void KeepRoad();

	int8 PersonalId = 0;
	int32 ImageId = 0;

	// Called to bind functionality to input
	UFUNCTION(BlueprintCallable, Category="Movement")
		void MoveForward(float value);
	UFUNCTION(BlueprintCallable, Category = "Movement")
		void Steer(float value);
	UFUNCTION(BlueprintCallable, Category = "Movement")
		void HandBreak();
	UFUNCTION(BlueprintCallable, Category = "Movement")
		void HandBreakReleased();
	UFUNCTION(BlueprintCallable, Category = "Movement")
		void ResetCar();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DesiredSpeed = 80.f;

protected:
	UPROPERTY(EditAnywhere)
		USceneComponent* FrontPoint = nullptr;
	UPROPERTY(EditAnywhere)
		USceneComponent* BackPoint = nullptr;
	UPROPERTY(EditAnywhere)
		USceneComponent* AdvancePoint = nullptr;
	UPROPERTY(EditAnywhere)
		USceneCaptureComponent2D* SceneCaptureComponent = nullptr;
	UPROPERTY(EditDefaultsOnly)
		ARoad* Road = nullptr;
	UPROPERTY(EditDefaultsOnly)
		float CriticalAngle = 0.5;
	UPROPERTY(EditDefaultsOnly)
		float MaxSpeed = 100;
	UPROPERTY(EditDefaultsOnly)
		bool DrawLine = false;
	UPROPERTY(EditDefaultsOnly)
		int32 VideoWidth = 512;
	UPROPERTY(EditDefaultsOnly)
		int32 VideoHeight = 128;
private:
	float PrevSpeedError = 30.f;

	float Kp = 0.5;
	float Kd = 0.01;

	
private:
	UPROPERTY()
		class UChaosWheeledVehicleMovementComponent* ChaosWheeledVehicleComponent = nullptr;
	UPROPERTY()
		UTextureRenderTarget2D* RenderTarget = nullptr;
	UPROPERTY()
		FString ImageFilePath;
	UPROPERTY()
		FString extension;

	EImageFormat ImageFormat;

	TSharedPtr<IImageWrapper> ImageWrapper = nullptr;
	
	FString CsvFilePath;

	bool BreakLightsState = false;

	bool WriteRowToCSV(const FString& FilePath, const TArray<FString>& Row);

	bool SaveCameraViewToDisk(const FString& FilePath);

	void SaveTrainingData();

};
