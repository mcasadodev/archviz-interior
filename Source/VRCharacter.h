// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NavigationSystem.h"

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

UCLASS()
class VRARCHVIZ_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class USceneComponent *VRRoot;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent *DestinationMarker;

	UPROPERTY(VisibleAnywhere)
	class USplineComponent *TeleportPath;

private:
	UPROPERTY(VisibleAnywhere)
	class UCameraComponent *Camera;

	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent *LeftController;

	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent *RightController;

	UPROPERTY(VisibleAnywhere)
	class UPostProcessComponent *PostProcessComponent;

	//UPROPERTY()
	//class UMaterialInstanceDynamic *BlinkerMaterialInstance;

private:
	UPROPERTY(EditAnywhere)
	bool Blinker = false;
	UPROPERTY(EditAnywhere)
	bool TeleportFade = false;
	UPROPERTY(EditAnywhere)
	bool UseControllers = true;

	UPROPERTY(EditAnywhere)
	float TeleportFadeTime = 1;

	UPROPERTY(EditAnywhere)
	float MaxTeleportDistance = 1000;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 800;
	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 10;
	UPROPERTY(EditAnywhere)
	float TeleportSimulationTime = 2;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(0, 0, 0);

	//UPROPERTY(EditAnywhere)
	//class UMaterialInterface *BlinkerMaterialBase;

	UPROPERTY(EditAnywhere)
	TArray<class USplineMeshComponent*> TeleportPathMeshPool;

	UPROPERTY(EditAnywhere)
	class UCurveFloat *RadiusVsVelocity;

	UPROPERTY(EditDefaultsOnly)
	class UStaticMesh *TeleportArchMesh;
	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface *TeleportArchMaterialON;
	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface *TeleportArchMaterialOFF;

private:
	bool FindTeleportDestinationNoControllers(FVector &OutLocation);
	bool FindTeleportDestinationWithControllers(TArray<FVector> &OutPath, FVector &OutLocation);
	bool HasDestination;

	void UpdateDestinationMarker();
	// void UpdateBlinker();
	// FVector2D GetBlinkerCenter();
	void UpdateSpline(const TArray<FVector> &Path);
	void DrawTeleportPath(const TArray<FVector> &Path);

	void MoveForward(float throttle);
	void MoveRight(float throttle);

	void RotateX(float throttle);
	void RotateY(float throttle);

	void BeginTeleport();
	void FinishTeleport();

	void StartFade(float FromAlpha, float ToAlpha);
};
