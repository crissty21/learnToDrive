// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "Road.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "TrainingDataCapturer.h"

#include "VehiclePawn.generated.h"

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
	UPROPERTY(EditDefaultsOnly)
		int8 TickingFreq = 1;
	UPROPERTY(EditAnywhere)
		int8 PersonalID = 0;

	UFUNCTION()
		float GetSteering();
	UFUNCTION()
		float GetThrottle();
	UFUNCTION()
		float GetBreak();
	UFUNCTION()
		float GetSpeed();

protected:
	UPROPERTY(EditAnywhere)
		USceneComponent* FrontPoint = nullptr;
	UPROPERTY(EditAnywhere)
		USceneComponent* BackPoint = nullptr;
	UPROPERTY(EditAnywhere)
		USceneComponent* AdvancePoint = nullptr;

	UPROPERTY(EditAnywhere)
		UTrainingDataCapturer* TrainingDataCapturer = nullptr;


	UPROPERTY(EditDefaultsOnly)
		ARoad* Road = nullptr;
	UPROPERTY(EditDefaultsOnly)
		float CriticalAngle = 0.5;
	UPROPERTY(EditDefaultsOnly)
		float MaxSpeed = 100;
	UPROPERTY(EditDefaultsOnly)
		float BreakTolerance = 0.5f;
	UPROPERTY(EditDefaultsOnly)
		bool DrawLine = false;
	UPROPERTY(EditDefaultsOnly)
		bool SaveData = false;

	UPROPERTY(EditAnywhere, Category = "PID")
		float Kp = 1.0f;
	UPROPERTY(EditAnywhere, Category = "PID")
		float Ki = 0.0f;
	UPROPERTY(EditAnywhere, Category = "PID")
		float Kd = 0.0f;
private:
	float PrevSpeedError = 30.f;

	
	float IntegralError = 0.0f;

	UPROPERTY()
		class UChaosWheeledVehicleMovementComponent* ChaosWheeledVehicleComponent = nullptr;

	bool BreakLightsState = false;

};
