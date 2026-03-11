// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystems/QMEnvironmentWorldSubsystem.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/QMPawnTrainingConfigComponent.h"
#include "Data/QMTrainingSettingsTypes.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/QMTrainingRuntimeDeveloperSettings.h"
#include "Subsystems/QMSettingsSubsystem.h"

void UQMEnvironmentWorldSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
	Super::Initialize(Collection);

	if (UQMSettingsSubsystem *SettingsSubsystem = GetSettingsSubsystem())
	{
		SettingsReloadedHandle = SettingsSubsystem->OnSettingsReloaded().AddUObject(
			this,
			&UQMEnvironmentWorldSubsystem::HandleSettingsReloaded);
	}
}

void UQMEnvironmentWorldSubsystem::Deinitialize()
{
	if (UQMSettingsSubsystem *SettingsSubsystem = GetSettingsSubsystem())
	{
		SettingsSubsystem->OnSettingsReloaded().Remove(SettingsReloadedHandle);
	}

	Super::Deinitialize();
}

void UQMEnvironmentWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const UQMTrainingRuntimeDeveloperSettings *RuntimeSettings = GetDefault<UQMTrainingRuntimeDeveloperSettings>();
	if (RuntimeSettings->bAutoApplyWorldSettingsOnBeginPlay)
	{
		ApplyWorldSettings();
	}
}

bool UQMEnvironmentWorldSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UQMEnvironmentWorldSubsystem::ApplyWorldSettings()
{
	UQMSettingsSubsystem *SettingsSubsystem = GetSettingsSubsystem();
	if (!SettingsSubsystem)
	{
		return;
	}

	const FQMWorldSettings &WorldSettings = SettingsSubsystem->GetWorldSettings();
	ApplyPawnSettings(WorldSettings.Pawn);
	ApplyCameraSettings(WorldSettings.Camera);
	ApplyLightingSettings(WorldSettings.Lighting);
	ApplyAssetPlacements(WorldSettings.AssetPlacements);
}

void UQMEnvironmentWorldSubsystem::ApplyPawnSettings(const FQMPawnWorldSettings &Settings)
{
	UWorld *World = GetWorld();
	if (!World)
	{
		return;
	}

	APawn *Pawn = Cast<APawn>(FindActorByTag(APawn::StaticClass(), Settings.TargetTag));
	if (!Pawn)
	{
		Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	}

	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("World settings: no pawn found to apply pawn settings."));
		return;
	}

	if (UQMPawnTrainingConfigComponent *ConfigComponent = Pawn->FindComponentByClass<UQMPawnTrainingConfigComponent>())
	{
		ConfigComponent->ApplyPawnSettings(Settings);
	}
	else
	{
		if (Settings.bApplyTransform)
		{
			Pawn->SetActorTransform(Settings.Transform.ToTransform());
		}

		if (Settings.bApplyMaxSpeed)
		{
			if (ACharacter *Character = Cast<ACharacter>(Pawn))
			{
				if (UCharacterMovementComponent *CharacterMovement = Character->GetCharacterMovement())
				{
					CharacterMovement->MaxWalkSpeed = Settings.MaxSpeed;
					CharacterMovement->MaxFlySpeed = Settings.MaxSpeed;
				}
			}
			else if (UFloatingPawnMovement *FloatingMovement = Pawn->FindComponentByClass<UFloatingPawnMovement>())
			{
				FloatingMovement->MaxSpeed = Settings.MaxSpeed;
			}
		}
	}
}

void UQMEnvironmentWorldSubsystem::ApplyCameraSettings(const FQMCameraWorldSettings &Settings)
{
	UCameraComponent *CameraComponent = nullptr;
	AActor *CameraOwner = nullptr;

	if (AActor *TaggedCameraActor = FindActorByTag(ACameraActor::StaticClass(), Settings.TargetTag))
	{
		if (ACameraActor *CameraActor = Cast<ACameraActor>(TaggedCameraActor))
		{
			CameraComponent = CameraActor->GetCameraComponent();
			CameraOwner = CameraActor;
		}
	}

	if (!CameraComponent)
	{
		if (AActor *TaggedActor = FindActorByTag(AActor::StaticClass(), Settings.TargetTag))
		{
			CameraComponent = TaggedActor->FindComponentByClass<UCameraComponent>();
			CameraOwner = TaggedActor;
		}
	}

	if (!CameraComponent)
	{
		UWorld *World = GetWorld();
		if (World && !Settings.TargetTag.IsNone())
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor *Actor = *It;
				if (!Actor)
				{
					continue;
				}

				TInlineComponentArray<UCameraComponent *> CameraComponents(Actor);
				for (UCameraComponent *Candidate : CameraComponents)
				{
					if (Candidate && Candidate->ComponentTags.Contains(Settings.TargetTag))
					{
						CameraComponent = Candidate;
						CameraOwner = Actor;
						break;
					}
				}

				if (CameraComponent)
				{
					break;
				}
			}
		}
	}

	if (!CameraComponent)
	{
		UWorld *World = GetWorld();
		if (World)
		{
			if (APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
			{
				CameraComponent = PlayerPawn->FindComponentByClass<UCameraComponent>();
				CameraOwner = PlayerPawn;
			}
		}
	}

	if (!CameraComponent)
	{
		UE_LOG(LogTemp, Verbose, TEXT("World settings: no camera found for tag '%s'."), *Settings.TargetTag.ToString());
		return;
	}

	if (Settings.bApplyFieldOfView)
	{
		CameraComponent->SetFieldOfView(Settings.FieldOfView);
	}

	if (Settings.bApplyPostProcess)
	{
		CameraComponent->PostProcessSettings.bOverride_BloomIntensity = true;
		CameraComponent->PostProcessSettings.BloomIntensity = Settings.BloomIntensity;

		CameraComponent->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
		CameraComponent->PostProcessSettings.AutoExposureMinBrightness = Settings.AutoExposureMinBrightness;

		CameraComponent->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
		CameraComponent->PostProcessSettings.AutoExposureMaxBrightness = Settings.AutoExposureMaxBrightness;
	}

	if (CameraOwner)
	{
		UE_LOG(LogTemp, Verbose, TEXT("World settings: applied camera settings to '%s'."), *CameraOwner->GetName());
	}
}

void UQMEnvironmentWorldSubsystem::ApplyLightingSettings(const FQMLightingWorldSettings &Settings)
{
	AActor *LightOwner = FindActorByTag(ADirectionalLight::StaticClass(), Settings.DirectionalLightTag);
	ADirectionalLight *DirectionalLight = Cast<ADirectionalLight>(LightOwner);
	if (!DirectionalLight)
	{
		UE_LOG(LogTemp, Verbose, TEXT("World settings: no directional light found for tag '%s'."), *Settings.DirectionalLightTag.ToString());
		return;
	}

	if (Settings.bApplyRotation)
	{
		DirectionalLight->SetActorRotation(Settings.Rotation);
	}

	if (Settings.bApplyIntensity && DirectionalLight->GetLightComponent())
	{
		DirectionalLight->GetLightComponent()->SetIntensity(Settings.Intensity);
	}
}

void UQMEnvironmentWorldSubsystem::ApplyAssetPlacements(const TArray<FQMAssetPlacementWorldSettings> &Placements)
{
	UWorld *World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const FQMAssetPlacementWorldSettings &Placement : Placements)
	{
		AActor *TargetActor = FindActorByTag(AActor::StaticClass(), Placement.TargetTag);

		if (!TargetActor && Placement.bSpawnIfMissing && !Placement.ActorClassPath.IsEmpty())
		{
			FSoftClassPath ClassPath(Placement.ActorClassPath);
			if (UClass *ActorClass = ClassPath.TryLoadClass<AActor>())
			{
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (!Placement.SpawnName.IsNone())
				{
					SpawnParameters.Name = Placement.SpawnName;
				}

				TargetActor = World->SpawnActor<AActor>(
					ActorClass,
					Placement.Transform.ToTransform(),
					SpawnParameters);

				if (TargetActor && !Placement.TargetTag.IsNone())
				{
					TargetActor->Tags.AddUnique(Placement.TargetTag);
				}
			}
			else
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("World settings: failed loading actor class '%s' for asset placement."),
					*Placement.ActorClassPath);
			}
		}

		if (TargetActor)
		{
			TargetActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			TargetActor->SetActorTransform(Placement.Transform.ToTransform());
		}
	}
}

AActor *UQMEnvironmentWorldSubsystem::FindActorByTag(UClass *ActorClass, FName Tag) const
{
	UWorld *World = GetWorld();
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
	{
		AActor *Actor = *It;
		if (Tag.IsNone() || Actor->Tags.Contains(Tag))
		{
			return Actor;
		}
	}

	return nullptr;
}

UQMSettingsSubsystem *UQMEnvironmentWorldSubsystem::GetSettingsSubsystem() const
{
	const UWorld *World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance *GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UQMSettingsSubsystem>() : nullptr;
}

void UQMEnvironmentWorldSubsystem::HandleSettingsReloaded()
{
	const UQMTrainingRuntimeDeveloperSettings *RuntimeSettings = GetDefault<UQMTrainingRuntimeDeveloperSettings>();
	if (RuntimeSettings->bAutoApplyWorldSettingsOnBeginPlay)
	{
		ApplyWorldSettings();
	}
}
