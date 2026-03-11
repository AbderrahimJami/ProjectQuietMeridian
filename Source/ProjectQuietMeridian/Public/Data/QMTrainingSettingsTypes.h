// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QMTrainingSettingsTypes.generated.h"

USTRUCT(BlueprintType)
struct PROJECTQUIETMERIDIAN_API FQMTransformSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
	FVector Scale3D = FVector(1.0f, 1.0f, 1.0f);

	FTransform ToTransform() const
	{
		return FTransform(Rotation, Location, Scale3D);
	}
};

USTRUCT(BlueprintType)
struct PROJECTQUIETMERIDIAN_API FQMPawnWorldSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pawn")
	FName TargetTag = TEXT("TrainingPawn");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pawn")
	bool bApplyTransform = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pawn", meta = (EditCondition = "bApplyTransform"))
	FQMTransformSettings Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pawn")
	bool bApplyMaxSpeed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pawn", meta = (EditCondition = "bApplyMaxSpeed", ClampMin = "0.0"))
	float MaxSpeed = 1200.0f;
};

USTRUCT(BlueprintType)
struct PROJECTQUIETMERIDIAN_API FQMCameraWorldSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FName TargetTag = TEXT("TrainingCamera");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bApplyFieldOfView = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (EditCondition = "bApplyFieldOfView"))
	float FieldOfView = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bApplyPostProcess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|PostProcess", meta = (EditCondition = "bApplyPostProcess"))
	float BloomIntensity = 0.675f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|PostProcess", meta = (EditCondition = "bApplyPostProcess", ClampMin = "0.0"))
	float AutoExposureMinBrightness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|PostProcess", meta = (EditCondition = "bApplyPostProcess", ClampMin = "0.0"))
	float AutoExposureMaxBrightness = 1.0f;
};

USTRUCT(BlueprintType)
struct PROJECTQUIETMERIDIAN_API FQMLightingWorldSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	FName DirectionalLightTag = TEXT("SunLight");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bApplyRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bApplyRotation"))
	FRotator Rotation = FRotator(-45.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bApplyIntensity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bApplyIntensity", ClampMin = "0.0"))
	float Intensity = 10.0f;
};

USTRUCT(BlueprintType)
struct PROJECTQUIETMERIDIAN_API FQMAssetPlacementWorldSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FName TargetTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	bool bSpawnIfMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (EditCondition = "bSpawnIfMissing"))
	FString ActorClassPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FName SpawnName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FQMTransformSettings Transform;
};

USTRUCT(BlueprintType)
struct PROJECTQUIETMERIDIAN_API FQMWorldSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	int32 SchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	FQMPawnWorldSettings Pawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	FQMCameraWorldSettings Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	FQMLightingWorldSettings Lighting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	TArray<FQMAssetPlacementWorldSettings> AssetPlacements;
};

USTRUCT(BlueprintType)
struct PROJECTQUIETMERIDIAN_API FQMTelemetrySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	int32 SchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry", meta = (ClampMin = "0.01"))
	float IntervalSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	FString OutputRelativePath = TEXT("Saved/Telemetry/telemetry.jsonl");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	FString OutputDirectoryRelativePath = TEXT("Saved/Telemetry/Runs");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	FString OutputFilePrefix = TEXT("telemetry");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bCreateTimestampedFilePerRun = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bOverwriteOnStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bIncludeUtcTimestamp = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bIncludeSimulationTime = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bIncludePawnVelocity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	FName PawnTag = TEXT("TrainingPawn");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	TArray<FName> TrackedActorTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry", meta = (ClampMin = "1"))
	int32 MaxTrackedActors = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bIncludeTelemetryProviders = true;
};
