// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/QMTrainingRuntimeDeveloperSettings.h"

UQMTrainingRuntimeDeveloperSettings::UQMTrainingRuntimeDeveloperSettings()
{
	TelemetrySettingsPath = TEXT("Config/Training/TelemetrySettings.json");
	WorldSettingsPath = TEXT("Config/Training/WorldSettings.json");
}

FName UQMTrainingRuntimeDeveloperSettings::GetCategoryName() const
{
	return TEXT("Project");
}
