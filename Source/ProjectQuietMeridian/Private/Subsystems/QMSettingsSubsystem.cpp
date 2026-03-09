// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystems/QMSettingsSubsystem.h"

#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/QMTrainingRuntimeDeveloperSettings.h"

namespace QMSettingsSubsystemInternal
{
	template <typename TSettingsStruct>
	bool ParseStructFromRoot(
		const TSharedPtr<FJsonObject>& RootObject,
		const FString& NestedObjectField,
		TSettingsStruct& OutStruct)
	{
		if (!RootObject.IsValid())
		{
			return false;
		}

		const TSharedPtr<FJsonObject> StructRoot = RootObject->HasTypedField<EJson::Object>(NestedObjectField)
			? RootObject->GetObjectField(NestedObjectField)
			: RootObject;

		return FJsonObjectConverter::JsonObjectToUStruct(StructRoot.ToSharedRef(), &OutStruct, 0, 0);
	}
}

void UQMSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadSettings();
}

void UQMSettingsSubsystem::Deinitialize()
{
	SettingsReloaded.Clear();
	Super::Deinitialize();
}

bool UQMSettingsSubsystem::ReloadSettings()
{
	const UQMTrainingRuntimeDeveloperSettings* RuntimeSettings = GetDefault<UQMTrainingRuntimeDeveloperSettings>();
	TelemetrySettingsAbsolutePath = ResolveSettingsPath(RuntimeSettings->TelemetrySettingsPath);
	WorldSettingsAbsolutePath = ResolveSettingsPath(RuntimeSettings->WorldSettingsPath);

	FQMTelemetrySettings LoadedTelemetrySettings;
	FQMWorldSettings LoadedWorldSettings;

	const bool bTelemetryLoaded = LoadTelemetrySettings(TelemetrySettingsAbsolutePath, LoadedTelemetrySettings);
	const bool bWorldLoaded = LoadWorldSettings(WorldSettingsAbsolutePath, LoadedWorldSettings);

	if (bTelemetryLoaded)
	{
		TelemetrySettings = LoadedTelemetrySettings;
	}

	if (bWorldLoaded)
	{
		WorldSettings = LoadedWorldSettings;
	}

	SettingsReloaded.Broadcast();

	return bTelemetryLoaded || bWorldLoaded;
}

bool UQMSettingsSubsystem::LoadTelemetrySettings(const FString& AbsolutePath, FQMTelemetrySettings& OutSettings) const
{
	TSharedPtr<FJsonObject> RootObject;
	if (!LoadSettingsFile(AbsolutePath, RootObject))
	{
		return false;
	}

	const bool bParsed = QMSettingsSubsystemInternal::ParseStructFromRoot(
		RootObject,
		TEXT("Telemetry"),
		OutSettings);

	if (!bParsed)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed parsing telemetry settings from '%s'."), *AbsolutePath);
	}

	return bParsed;
}

bool UQMSettingsSubsystem::LoadWorldSettings(const FString& AbsolutePath, FQMWorldSettings& OutSettings) const
{
	TSharedPtr<FJsonObject> RootObject;
	if (!LoadSettingsFile(AbsolutePath, RootObject))
	{
		return false;
	}

	const bool bParsed = QMSettingsSubsystemInternal::ParseStructFromRoot(
		RootObject,
		TEXT("World"),
		OutSettings);

	if (!bParsed)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed parsing world settings from '%s'."), *AbsolutePath);
	}

	return bParsed;
}

bool UQMSettingsSubsystem::LoadSettingsFile(const FString& AbsolutePath, TSharedPtr<FJsonObject>& OutRootObject) const
{
	if (!FPaths::FileExists(AbsolutePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Settings file not found: %s"), *AbsolutePath);
		return false;
	}

	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed reading settings file: %s"), *AbsolutePath);
		return false;
	}

	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(JsonReader, OutRootObject) || !OutRootObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed deserializing JSON file: %s"), *AbsolutePath);
		return false;
	}

	return true;
}

FString UQMSettingsSubsystem::ResolveSettingsPath(const FString& ConfiguredPath) const
{
	if (ConfiguredPath.IsEmpty())
	{
		return FString();
	}

	if (FPaths::IsRelative(ConfiguredPath))
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ConfiguredPath);
	}

	return FPaths::ConvertRelativePathToFull(ConfiguredPath);
}
