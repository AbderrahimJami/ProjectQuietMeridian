// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "QMTrainingRuntimeDeveloperSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "QM Training Runtime"))
class PROJECTQUIETMERIDIAN_API UQMTrainingRuntimeDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UQMTrainingRuntimeDeveloperSettings();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category = "JSON Files", meta = (RelativeToGameDir))
	FString TelemetrySettingsPath;

	UPROPERTY(Config, EditAnywhere, Category = "JSON Files", meta = (RelativeToGameDir))
	FString WorldSettingsPath;

	UPROPERTY(Config, EditAnywhere, Category = "Runtime")
	bool bAutoApplyWorldSettingsOnBeginPlay = true;

	UPROPERTY(Config, EditAnywhere, Category = "Runtime")
	bool bAutoStartTelemetryOnBeginPlay = true;
};
