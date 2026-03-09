// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/QMTrainingSettingsTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "QMSettingsSubsystem.generated.h"

class FJsonObject;

DECLARE_MULTICAST_DELEGATE(FQMOnSettingsReloaded);

UCLASS()
class PROJECTQUIETMERIDIAN_API UQMSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool ReloadSettings();

	const FQMTelemetrySettings& GetTelemetrySettings() const { return TelemetrySettings; }
	const FQMWorldSettings& GetWorldSettings() const { return WorldSettings; }

	const FString& GetTelemetrySettingsAbsolutePath() const { return TelemetrySettingsAbsolutePath; }
	const FString& GetWorldSettingsAbsolutePath() const { return WorldSettingsAbsolutePath; }

	FQMOnSettingsReloaded& OnSettingsReloaded() { return SettingsReloaded; }

private:
	bool LoadTelemetrySettings(const FString& AbsolutePath, FQMTelemetrySettings& OutSettings) const;
	bool LoadWorldSettings(const FString& AbsolutePath, FQMWorldSettings& OutSettings) const;
	bool LoadSettingsFile(const FString& AbsolutePath, TSharedPtr<FJsonObject>& OutRootObject) const;
	FString ResolveSettingsPath(const FString& ConfiguredPath) const;

private:
	FQMTelemetrySettings TelemetrySettings;
	FQMWorldSettings WorldSettings;
	FString TelemetrySettingsAbsolutePath;
	FString WorldSettingsAbsolutePath;
	FQMOnSettingsReloaded SettingsReloaded;
};
