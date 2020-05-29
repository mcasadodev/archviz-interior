// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VRRoot);
	LeftController->SetTrackingSource(EControllerHand::Left);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VRRoot);
	RightController->SetTrackingSource(EControllerHand::Right);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(RightController);

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	DestinationMarker->SetVisibility(false);

	/*
	if (BlinkerMaterialBase != nullptr)
	{
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
	}
	*/
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	// FVector::VectorPlaneProject();
	NewCameraOffset.Z = 0;
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset); // + FVector(VRRoot->GetComponentLocation().X -20, 0, VRRoot->GetComponentLocation().Z));

	UpdateDestinationMarker();
	if (Blinker)
	{
		// UpdateBlinker();
	}
	// UpdateBlinker();
}

bool AVRCharacter::FindTeleportDestinationNoControllers(FVector &OutLocation)
{
	LeftController->SetVisibility(false);
	RightController->SetVisibility(false);

	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * MaxTeleportDistance;

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);

	if (!bHit)
		return false;

	FNavLocation NavLocation;

	UNavigationSystemV1 *NavSystem = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
	bool bOnNavMesh = false;
	if (NavSystem != nullptr)
	{
		bOnNavMesh = NavSystem->ProjectPointToNavigation(
			HitResult.Location,
			NavLocation,
			TeleportProjectionExtent);
	}

	if (!bOnNavMesh)
		return false;

	OutLocation = NavLocation.Location;

	return true;
}

bool AVRCharacter::FindTeleportDestinationWithControllers(TArray<FVector> &OutPath, FVector &OutLocation)
{
	FVector Start = RightController->GetComponentLocation();
	FVector Look = RightController->GetForwardVector();
	// Look = Look.RotateAngleAxis(45, RightController->GetRightVector());
	FVector End = Start + Look * MaxTeleportDistance;

	FPredictProjectilePathParams Params(
		TeleportProjectileRadius,
		Start,
		Look * TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility,
		this);
	// Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	Params.bTraceComplex = true;
	FPredictProjectilePathResult Result;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	if (!bHit)
		return false;

	for (FPredictProjectilePathPointData PointData : Result.PathData)
	{
		OutPath.Add(PointData.Location);
	}

	FNavLocation NavLocation;

	UNavigationSystemV1 *NavSystem = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
	bool bOnNavMesh = false;
	if (NavSystem != nullptr)
	{
		bOnNavMesh = NavSystem->ProjectPointToNavigation(
			Result.HitResult.Location,
			NavLocation,
			TeleportProjectionExtent);
	}

	if (!bOnNavMesh)
		return false;

	OutLocation = NavLocation.Location;

	return bHit && bOnNavMesh;
}

void AVRCharacter::UpdateDestinationMarker()
{
	TArray<FVector> Path;
	FVector Location;
	bool bHasDestination;
	if (!UseControllers)
	{
		bHasDestination = FindTeleportDestinationNoControllers(Location);
	}
	else
	{
		bHasDestination = FindTeleportDestinationWithControllers(Path, Location);
	}

	HasDestination = bHasDestination;

	if (bHasDestination)
	{
		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(Location);

		if (UseControllers)
		{
			DrawTeleportPath(Path);
		}
	}
	else
	{
		DestinationMarker->SetVisibility(false);

		if (UseControllers)
		{
			// TArray<FVector> EmptyPath;
			// DrawTeleportPath(EmptyPath);

			DrawTeleportPath(Path);
		}
	}
}
/*
void AVRCharacter::UpdateBlinker()
{
	if (RadiusVsVelocity == nullptr)
		return;

	float Speed = GetVelocity().Size();
	float Radius = RadiusVsVelocity->GetFloatValue(Speed);

	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);

	FVector2D Center = GetBlinkerCenter();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0));
}

FVector2D AVRCharacter::GetBlinkerCenter()
{
	// FVector MovementDirection = GetVelocity().GetSafeNormal();
	FVector MovementDirection = GetVelocity();

	if (MovementDirection.IsNearlyZero() || MovementDirection == FVector(0, 0, 0))
	{
		return FVector2D(0.5, 0.5);
	}

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0)
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;
	}

	APlayerController *PC = Cast<APlayerController>(GetController());

	if (PC == nullptr)
	{
		return FVector2D(0.5, 0.5);
	}

	FVector2D ScreenStationaryLocation;
	PC->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	ScreenStationaryLocation.X /= SizeX;
	ScreenStationaryLocation.Y /= SizeY;

	return ScreenStationaryLocation;
}
*/
void AVRCharacter::DrawTeleportPath(const TArray<FVector> &Path)
{

	UpdateSpline(Path);

	for (USplineMeshComponent *SplineMesh : TeleportPathMeshPool)
	{
		SplineMesh->SetVisibility(false);
	}

	int32 SegmentNum = Path.Num() - 1;
	for (int32 i = 0; i < SegmentNum; i++)
	{
		if (TeleportPathMeshPool.Num() <= i)
		{
			USplineMeshComponent *SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(TeleportArchMesh);
			// SplineMesh->SetMaterial(0, TeleportArchMaterial);
			SplineMesh->RegisterComponent();
			TeleportPathMeshPool.Add(SplineMesh);
		}

		USplineMeshComponent *SplineMesh = TeleportPathMeshPool[i];
		SplineMesh->SetVisibility(true);

		FVector StartPos, StartTangent, EndPos, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent);
		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);

		if (HasDestination)
		{
			SplineMesh->SetMaterial(0, TeleportArchMaterialON);
		}
		else
		{
			SplineMesh->SetMaterial(0, TeleportArchMaterialOFF);
		}
	}
}

void AVRCharacter::UpdateSpline(const TArray<FVector> &Path)
{
	TeleportPath->ClearSplinePoints(false);

	for (int32 i = 0; i < Path.Num(); i++)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}

	TeleportPath->UpdateSpline();
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Rotate X"), this, &AVRCharacter::RotateX);
	PlayerInputComponent->BindAxis(TEXT("Rotate Y"), this, &AVRCharacter::RotateY);
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Pressed, this, &AVRCharacter::BeginTeleport);
}

void AVRCharacter::MoveForward(float throttle)
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}

void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}

void AVRCharacter::RotateX(float throttle)
{
	AddControllerYawInput(throttle);
}

void AVRCharacter::RotateY(float throttle)
{
	FRotator cc = FRotator(throttle, 0, 0);
	Camera->AddLocalRotation(cc);
}

void AVRCharacter::BeginTeleport()
{
	TArray<FVector> Path;
	FVector Location;
	bool bHasDestination;
	if (!UseControllers)
	{
		bHasDestination = FindTeleportDestinationNoControllers(Location);
	}
	else
	{
		bHasDestination = FindTeleportDestinationWithControllers(Path, Location);
	}

	if (bHasDestination)
	{
		if (TeleportFade)
		{
			StartFade(0, 1);

			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportFadeTime);
		}
		else
		{
			//SetActorLocation(DestinationMarker->GetComponentLocation() + FVector(0, 0, 88));
			FVector Destination = DestinationMarker->GetComponentLocation();
			Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
			SetActorLocation(Destination);
		}
	}
}

void AVRCharacter::FinishTeleport()
{
	//SetActorLocation(DestinationMarker->GetComponentLocation() + FVector(0, 0, 88));
	FVector Destination = DestinationMarker->GetComponentLocation();
	Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
	SetActorLocation(Destination);
	StartFade(1, 0);
}

void AVRCharacter::StartFade(float FromAlpha, float ToAlpha)
{
	APlayerController *PC = Cast<APlayerController>(GetController());

	if (PC != nullptr)
	{
		// PC->PlayerCameraManager->StartCameraFade(1, 0, TeleportFadeTime, FLinearColor::Black);
		PC->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black);
	}
}