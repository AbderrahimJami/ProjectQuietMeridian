// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "QMEnvironmentWorldSubsystem.generated.h"

class AActor;
class UQMSettingsSubsystem;

struct FQMAssetPlacementWorldSettings;
struct FQMCameraWorldSettings;
struct FQMLightingWorldSettings;
struct FQMPawnWorldSettings;

UCLASS()
class PROJECTQUIETMERIDIAN_API UQMEnvironmentWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintCallable, Category = "QM|Environment")
	void ApplyWorldSettings();

private:
	void ApplyPawnSettings(const FQMPawnWorldSettings& Settings);
	void ApplyCameraSettings(const FQMCameraWorldSettings& Settings);
	void ApplyLightingSettings(const FQMLightingWorldSettings& Settings);
	void ApplyAssetPlacements(const TArray<FQMAssetPlacementWorldSettings>& Placements);

	AActor* FindActorByTag(UClass* ActorClass, FName Tag) const;
	UQMSettingsSubsystem* GetSettingsSubsystem() const;
	void HandleSettingsReloaded();

private:
	FDelegateHandle SettingsReloadedHandle;
};
