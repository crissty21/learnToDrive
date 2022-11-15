// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture.h"

#include "Interpreter.generated.h"

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
		UTexture2D* Texture = nullptr;
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

	// reads the current video frame
	UFUNCTION(BlueprintCallable, Category = Data)
		void ReadFrame();
};
