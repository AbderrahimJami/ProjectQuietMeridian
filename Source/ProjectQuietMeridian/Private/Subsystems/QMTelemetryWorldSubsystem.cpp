// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystems/QMTelemetryWorldSubsystem.h"

#include "Components/ActorComponent.h"
#include "Data/QMTrainingSettingsTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/QMTelemetryProvider.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/QMTrainingRuntimeDeveloperSettings.h"
#include "Subsystems/QMSettingsSubsystem.h"

namespace QMTelemetryWorldSubsystemInternal
{
	FString VectorToString(const FVector &InVector)
	{
		return FString::Printf(TEXT("%.4f,%.4f,%.4f"), InVector.X, InVector.Y, InVector.Z);
	}

	FString RotatorToString(const FRotator &InRotator)
	{
		return FString::Printf(TEXT("%.4f,%.4f,%.4f"), InRotator.Pitch, InRotator.Yaw, InRotator.Roll);
	}

	TSharedPtr<FJsonObject> BuildActorTransformPayload(const AActor *Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
		Payload->SetStringField(TEXT("name"), Actor->GetName());
		Payload->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
		Payload->SetStringField(TEXT("location"), VectorToString(Actor->GetActorLocation()));
		Payload->SetStringField(TEXT("rotation"), RotatorToString(Actor->GetActorRotation()));
		Payload->SetStringField(TEXT("scale"), VectorToString(Actor->GetActorScale3D()));
		return Payload;
	}
}

void UQMTelemetryWorldSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
	Super::Initialize(Collection);

	if (UQMSettingsSubsystem *SettingsSubsystem = GetSettingsSubsystem())
	{
		SettingsReloadedHandle = SettingsSubsystem->OnSettingsReloaded().AddUObject(
			this,
			&UQMTelemetryWorldSubsystem::HandleSettingsReloaded);
	}
}

void UQMTelemetryWorldSubsystem::Deinitialize()
{
	StopTelemetry();

	if (UQMSettingsSubsystem *SettingsSubsystem = GetSettingsSubsystem())
	{
		SettingsSubsystem->OnSettingsReloaded().Remove(SettingsReloadedHandle);
	}

	Super::Deinitialize();
}

void UQMTelemetryWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const UQMTrainingRuntimeDeveloperSettings *RuntimeSettings = GetDefault<UQMTrainingRuntimeDeveloperSettings>();
	if (RuntimeSettings->bAutoStartTelemetryOnBeginPlay)
	{
		StartTelemetry();
	}
}

bool UQMTelemetryWorldSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UQMTelemetryWorldSubsystem::StartTelemetry()
{
	UWorld *World = GetWorld();
	UQMSettingsSubsystem *SettingsSubsystem = GetSettingsSubsystem();
	if (!World || !SettingsSubsystem)
	{
		return;
	}

	const FQMTelemetrySettings &Settings = SettingsSubsystem->GetTelemetrySettings();
	StopTelemetry();

	if (!Settings.bEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("Telemetry start skipped because telemetry is disabled in settings."));
		return;
	}

	if (Settings.bCreateTimestampedFilePerRun)
	{
		OutputAbsolutePath = BuildRunScopedOutputPath(Settings);
	}
	else
	{
		OutputAbsolutePath = ResolveTelemetryOutputPath(Settings);
	}

	if (OutputAbsolutePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Telemetry output path is empty, cannot start telemetry logging."));
		return;
	}

	const FString OutputDirectory = FPaths::GetPath(OutputAbsolutePath);
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);

	if (!Settings.bCreateTimestampedFilePerRun && Settings.bOverwriteOnStart)
	{
		const bool bResetFile = FFileHelper::SaveStringToFile(
			TEXT(""),
			*OutputAbsolutePath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get());

		if (!bResetFile)
		{
			UE_LOG(LogTemp, Warning, TEXT("Telemetry failed to reset output file: %s"), *OutputAbsolutePath);
		}
	}

	ActiveTelemetrySettings = Settings;
	RebuildTelemetryCaches(ActiveTelemetrySettings);
	ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &UQMTelemetryWorldSubsystem::HandleActorSpawned));

	const float SafeInterval = FMath::Max(Settings.IntervalSeconds, 0.01f);
	World->GetTimerManager().SetTimer(
		CaptureTimerHandle,
		this,
		&UQMTelemetryWorldSubsystem::CaptureSnapshot,
		SafeInterval,
		true);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Telemetry started. Interval=%.3fs Output='%s'"),
		SafeInterval,
		*OutputAbsolutePath);
}

void UQMTelemetryWorldSubsystem::StopTelemetry()
{
	UWorld *World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(CaptureTimerHandle);
		if (ActorSpawnedHandle.IsValid())
		{
			World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		}
	}
	ActorSpawnedHandle.Reset();

	CachedTrackedPawn = nullptr;
	CachedTrackedActorsByTag.Empty();
	CachedProviderActors.Empty();
	CachedProviderComponents.Empty();
}

bool UQMTelemetryWorldSubsystem::IsTelemetryRunning() const
{
	UWorld *World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(CaptureTimerHandle);
}

void UQMTelemetryWorldSubsystem::CaptureSnapshot()
{
	UWorld *World = GetWorld();
	if (!World)
	{
		return;
	}

	const FQMTelemetrySettings &Settings = ActiveTelemetrySettings;
	PruneInvalidTelemetryCaches();

	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetNumberField(TEXT("frame"), static_cast<double>(GFrameCounter));

	if (Settings.bIncludeUtcTimestamp)
	{
		RootObject->SetStringField(TEXT("utc"), FDateTime::UtcNow().ToIso8601());
	}

	if (Settings.bIncludeSimulationTime)
	{
		RootObject->SetNumberField(TEXT("sim_time_seconds"), World->GetTimeSeconds());
	}

	if (APawn *Pawn = ResolveTrackedPawn(Settings))
	{
		TSharedPtr<FJsonObject> PawnPayload = QMTelemetryWorldSubsystemInternal::BuildActorTransformPayload(Pawn);
		if (PawnPayload.IsValid())
		{
			if (Settings.bIncludePawnVelocity)
			{
				PawnPayload->SetStringField(
					TEXT("velocity"),
					QMTelemetryWorldSubsystemInternal::VectorToString(Pawn->GetVelocity()));
			}

			RootObject->SetObjectField(TEXT("pawn"), PawnPayload);
		}
	}

	AppendTrackedActors(RootObject, Settings);

	if (Settings.bIncludeTelemetryProviders)
	{
		AppendTelemetryProviders(RootObject);
	}

	FString JsonLine;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonLine);
	if (!FJsonSerializer::Serialize(RootObject, Writer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Telemetry snapshot serialization failed."));
		return;
	}

	JsonLine.Append(LINE_TERMINATOR);
	if (!FFileHelper::SaveStringToFile(
			JsonLine,
			*OutputAbsolutePath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(),
			FILEWRITE_Append))
	{
		UE_LOG(LogTemp, Warning, TEXT("Telemetry write failed: %s"), *OutputAbsolutePath);
	}
}

void UQMTelemetryWorldSubsystem::HandleSettingsReloaded()
{
	if (IsTelemetryRunning())
	{
		StartTelemetry();
	}
}

void UQMTelemetryWorldSubsystem::AppendTrackedActors(const TSharedRef<FJsonObject> &RootObject, const FQMTelemetrySettings &Settings) const
{
	if (Settings.TrackedActorTags.IsEmpty())
	{
		return;
	}

	TArray<TSharedPtr<FJsonValue>> TrackedActors;
	TSet<const AActor *> SeenActors;

	for (const FName &Tag : Settings.TrackedActorTags)
	{
		if (Tag.IsNone())
		{
			continue;
		}

		const TArray<TWeakObjectPtr<AActor>> *CachedActors = CachedTrackedActorsByTag.Find(Tag);
		if (!CachedActors)
		{
			continue;
		}

		for (const TWeakObjectPtr<AActor> &WeakActor : *CachedActors)
		{
			const AActor *Actor = WeakActor.Get();
			if (!Actor || SeenActors.Contains(Actor) || !Actor->Tags.Contains(Tag))
			{
				continue;
			}

			TSharedPtr<FJsonObject> ActorPayload = QMTelemetryWorldSubsystemInternal::BuildActorTransformPayload(Actor);
			if (ActorPayload.IsValid())
			{
				ActorPayload->SetStringField(TEXT("matched_tag"), Tag.ToString());
				TrackedActors.Add(MakeShared<FJsonValueObject>(ActorPayload));
				SeenActors.Add(Actor);
			}

			if (TrackedActors.Num() >= Settings.MaxTrackedActors)
			{
				RootObject->SetArrayField(TEXT("tracked_actors"), TrackedActors);
				return;
			}
		}
	}

	RootObject->SetArrayField(TEXT("tracked_actors"), TrackedActors);
}

void UQMTelemetryWorldSubsystem::AppendTelemetryProviders(TSharedRef<FJsonObject> RootObject) const
{
	TArray<TSharedPtr<FJsonValue>> ProviderPayloads;

	for (const TWeakObjectPtr<AActor> &WeakActor : CachedProviderActors)
	{
		AActor *Actor = WeakActor.Get();
		if (!Actor)
		{
			continue;
		}

		if (Actor->GetClass()->ImplementsInterface(UQMTelemetryProvider::StaticClass()))
		{
			if (IQMTelemetryProvider *Provider = Cast<IQMTelemetryProvider>(Actor))
			{
				TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
				Payload->SetStringField(TEXT("source_type"), TEXT("actor"));
				Payload->SetStringField(TEXT("source_name"), Actor->GetName());
				Provider->GatherTelemetry(Payload);
				ProviderPayloads.Add(MakeShared<FJsonValueObject>(Payload));
			}
		}
	}

	for (const TWeakObjectPtr<UActorComponent> &WeakComponent : CachedProviderComponents)
	{
		UActorComponent *Component = WeakComponent.Get();
		if (!Component || !Component->GetClass()->ImplementsInterface(UQMTelemetryProvider::StaticClass()))
		{
			continue;
		}

		AActor *Owner = Component->GetOwner();
		if (IQMTelemetryProvider *Provider = Cast<IQMTelemetryProvider>(Component))
		{
			TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
			Payload->SetStringField(TEXT("source_type"), TEXT("component"));
			Payload->SetStringField(TEXT("source_name"), Component->GetName());
			Payload->SetStringField(TEXT("owner"), Owner ? Owner->GetName() : FString(TEXT("None")));
			Provider->GatherTelemetry(Payload);
			ProviderPayloads.Add(MakeShared<FJsonValueObject>(Payload));
		}
	}

	if (ProviderPayloads.Num() > 0)
	{
		RootObject->SetArrayField(TEXT("providers"), ProviderPayloads);
	}
}

APawn *UQMTelemetryWorldSubsystem::ResolveTrackedPawn(const FQMTelemetrySettings &Settings) const
{
	UWorld *World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (CachedTrackedPawn.IsValid())
	{
		return CachedTrackedPawn.Get();
	}

	APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn)
	{
		return nullptr;
	}

	if (Settings.PawnTag.IsNone() || PlayerPawn->Tags.Contains(Settings.PawnTag))
	{
		return PlayerPawn;
	}

	return nullptr;
}

FString UQMTelemetryWorldSubsystem::ResolveTelemetryOutputPath(const FQMTelemetrySettings &Settings) const
{
	if (Settings.OutputRelativePath.IsEmpty())
	{
		return FString();
	}

	if (FPaths::IsRelative(Settings.OutputRelativePath))
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / Settings.OutputRelativePath);
	}

	return FPaths::ConvertRelativePathToFull(Settings.OutputRelativePath);
}

FString UQMTelemetryWorldSubsystem::BuildRunScopedOutputPath(const FQMTelemetrySettings &Settings) const
{
	FString DirectoryPath = Settings.OutputDirectoryRelativePath;
	if (DirectoryPath.IsEmpty())
	{
		DirectoryPath = FPaths::GetPath(Settings.OutputRelativePath);
		if (DirectoryPath.IsEmpty())
		{
			DirectoryPath = TEXT("Saved/Telemetry/Runs");
		}
	}

	const FString AbsoluteDirectoryPath = FPaths::IsRelative(DirectoryPath)
											  ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / DirectoryPath)
											  : FPaths::ConvertRelativePathToFull(DirectoryPath);

	FString FilePrefix = Settings.OutputFilePrefix;
	if (FilePrefix.IsEmpty())
	{
		FilePrefix = FPaths::GetBaseFilename(Settings.OutputRelativePath);
		if (FilePrefix.IsEmpty())
		{
			FilePrefix = TEXT("telemetry");
		}
	}

	FString ExtensionWithDot = FPaths::GetExtension(Settings.OutputRelativePath, true);
	if (ExtensionWithDot.IsEmpty())
	{
		ExtensionWithDot = TEXT(".jsonl");
	}

	const FDateTime Timestamp = FDateTime::UtcNow();
	const FString Stamp = FString::Printf(
		TEXT("%04d%02d%02d_%02d%02d%02d"),
		Timestamp.GetYear(),
		Timestamp.GetMonth(),
		Timestamp.GetDay(),
		Timestamp.GetHour(),
		Timestamp.GetMinute(),
		Timestamp.GetSecond());

	FString CandidatePath = FPaths::Combine(
		AbsoluteDirectoryPath,
		FString::Printf(TEXT("%s_%s%s"), *FilePrefix, *Stamp, *ExtensionWithDot));

	int32 CollisionIndex = 1;
	while (FPaths::FileExists(CandidatePath))
	{
		CandidatePath = FPaths::Combine(
			AbsoluteDirectoryPath,
			FString::Printf(TEXT("%s_%s_%d%s"), *FilePrefix, *Stamp, CollisionIndex, *ExtensionWithDot));
		++CollisionIndex;
	}

	return CandidatePath;
}

UQMSettingsSubsystem *UQMTelemetryWorldSubsystem::GetSettingsSubsystem() const
{
	const UWorld *World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance *GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UQMSettingsSubsystem>() : nullptr;
}

void UQMTelemetryWorldSubsystem::RebuildTelemetryCaches(const FQMTelemetrySettings &Settings)
{
	UWorld *World = GetWorld();
	if (!World)
	{
		return;
	}

	CachedTrackedPawn = nullptr;
	CachedTrackedActorsByTag.Empty();
	CachedProviderActors.Empty();
	CachedProviderComponents.Empty();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		RegisterActorForTelemetry(*It, Settings);
	}
}

void UQMTelemetryWorldSubsystem::RegisterActorForTelemetry(AActor *Actor, const FQMTelemetrySettings &Settings)
{
	if (!Actor)
	{
		return;
	}

	if (!CachedTrackedPawn.IsValid() && !Settings.PawnTag.IsNone())
	{
		APawn *Pawn = Cast<APawn>(Actor);
		if (Pawn && Pawn->Tags.Contains(Settings.PawnTag))
		{
			CachedTrackedPawn = Pawn;
		}
	}

	for (const FName &TrackedTag : Settings.TrackedActorTags)
	{
		if (TrackedTag.IsNone() || !Actor->Tags.Contains(TrackedTag))
		{
			continue;
		}

		TArray<TWeakObjectPtr<AActor>> &CachedActorsForTag = CachedTrackedActorsByTag.FindOrAdd(TrackedTag);
		CachedActorsForTag.AddUnique(Actor);
	}

	if (!Settings.bIncludeTelemetryProviders)
	{
		return;
	}

	if (Actor->GetClass()->ImplementsInterface(UQMTelemetryProvider::StaticClass()))
	{
		CachedProviderActors.AddUnique(Actor);
	}

	const TSet<UActorComponent *> &Components = Actor->GetComponents();
	for (UActorComponent *Component : Components)
	{
		if (Component && Component->GetClass()->ImplementsInterface(UQMTelemetryProvider::StaticClass()))
		{
			CachedProviderComponents.AddUnique(Component);
		}
	}
}

void UQMTelemetryWorldSubsystem::HandleActorSpawned(AActor *SpawnedActor)
{
	RegisterActorForTelemetry(SpawnedActor, ActiveTelemetrySettings);
}

void UQMTelemetryWorldSubsystem::PruneInvalidTelemetryCaches()
{
	for (auto It = CachedTrackedActorsByTag.CreateIterator(); It; ++It)
	{
		TArray<TWeakObjectPtr<AActor>> &CachedActors = It.Value();
		CachedActors.RemoveAll([](const TWeakObjectPtr<AActor> &WeakActor)
							   { return !WeakActor.IsValid(); });

		if (CachedActors.IsEmpty())
		{
			It.RemoveCurrent();
		}
	}

	CachedProviderActors.RemoveAll([](const TWeakObjectPtr<AActor> &WeakActor)
								   { return !WeakActor.IsValid(); });
	CachedProviderComponents.RemoveAll([](const TWeakObjectPtr<UActorComponent> &WeakComponent)
									   { return !WeakComponent.IsValid(); });

	if (!CachedTrackedPawn.IsValid())
	{
		CachedTrackedPawn = nullptr;
	}
}
