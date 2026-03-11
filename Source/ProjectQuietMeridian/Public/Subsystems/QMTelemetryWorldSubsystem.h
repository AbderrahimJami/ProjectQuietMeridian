// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/QMTrainingSettingsTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "QMTelemetryWorldSubsystem.generated.h"

class AActor;
class APawn;
class FJsonObject;
class UActorComponent;
class UQMSettingsSubsystem;

UCLASS()
class PROJECTQUIETMERIDIAN_API UQMTelemetryWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintCallable, Category = "QM|Telemetry")
	void StartTelemetry();

	UFUNCTION(BlueprintCallable, Category = "QM|Telemetry")
	void StopTelemetry();

	UFUNCTION(BlueprintPure, Category = "QM|Telemetry")
	bool IsTelemetryRunning() const;

private:
	void CaptureSnapshot();
	void HandleSettingsReloaded();
	void AppendTrackedActors(const TSharedRef<FJsonObject>& RootObject, const FQMTelemetrySettings& Settings) const;
	void AppendTelemetryProviders(TSharedRef<FJsonObject> RootObject) const;
	APawn* ResolveTrackedPawn(const FQMTelemetrySettings& Settings) const;
	FString ResolveTelemetryOutputPath(const FQMTelemetrySettings& Settings) const;
	FString BuildRunScopedOutputPath(const FQMTelemetrySettings& Settings) const;
	UQMSettingsSubsystem* GetSettingsSubsystem() const;
	void RebuildTelemetryCaches(const FQMTelemetrySettings& Settings);
	void RegisterActorForTelemetry(AActor* Actor, const FQMTelemetrySettings& Settings);
	void HandleActorSpawned(AActor* SpawnedActor);
	void PruneInvalidTelemetryCaches();

private:
	FTimerHandle CaptureTimerHandle;
	FDelegateHandle SettingsReloadedHandle;
	FDelegateHandle ActorSpawnedHandle;
	FString OutputAbsolutePath;
	FQMTelemetrySettings ActiveTelemetrySettings;
	TWeakObjectPtr<APawn> CachedTrackedPawn;
	TMap<FName, TArray<TWeakObjectPtr<AActor>>> CachedTrackedActorsByTag;
	TArray<TWeakObjectPtr<AActor>> CachedProviderActors;
	TArray<TWeakObjectPtr<UActorComponent>> CachedProviderComponents;
};
