// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/QMSimpleTelemetryProviderComponent.h"

#include "Dom/JsonObject.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

namespace QMSimpleTelemetryProviderComponentInternal
{
	TSharedPtr<FJsonObject> MakeVectorPayload(const FVector& InVector)
	{
		TSharedPtr<FJsonObject> VectorPayload = MakeShared<FJsonObject>();
		VectorPayload->SetNumberField(TEXT("x"), InVector.X);
		VectorPayload->SetNumberField(TEXT("y"), InVector.Y);
		VectorPayload->SetNumberField(TEXT("z"), InVector.Z);
		return VectorPayload;
	}

	TSharedPtr<FJsonObject> MakeRotatorPayload(const FRotator& InRotator)
	{
		TSharedPtr<FJsonObject> RotatorPayload = MakeShared<FJsonObject>();
		RotatorPayload->SetNumberField(TEXT("pitch"), InRotator.Pitch);
		RotatorPayload->SetNumberField(TEXT("yaw"), InRotator.Yaw);
		RotatorPayload->SetNumberField(TEXT("roll"), InRotator.Roll);
		return RotatorPayload;
	}
}

UQMSimpleTelemetryProviderComponent::UQMSimpleTelemetryProviderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UQMSimpleTelemetryProviderComponent::GatherTelemetry(TSharedRef<FJsonObject> OutTelemetry) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	OutTelemetry->SetStringField(TEXT("provider_id"), TEXT("simple_provider"));
	OutTelemetry->SetStringField(TEXT("owner_class"), Owner->GetClass()->GetName());

	if (bIncludeWorldTimeSeconds)
	{
		if (const UWorld* World = GetWorld())
		{
			OutTelemetry->SetNumberField(TEXT("world_time_seconds"), World->GetTimeSeconds());
		}
	}

	if (bIncludeOwnerTransform)
	{
		OutTelemetry->SetObjectField(
			TEXT("location"),
			QMSimpleTelemetryProviderComponentInternal::MakeVectorPayload(Owner->GetActorLocation()));
		OutTelemetry->SetObjectField(
			TEXT("rotation"),
			QMSimpleTelemetryProviderComponentInternal::MakeRotatorPayload(Owner->GetActorRotation()));
	}

	if (bIncludeOwnerVelocity)
	{
		OutTelemetry->SetObjectField(
			TEXT("velocity"),
			QMSimpleTelemetryProviderComponentInternal::MakeVectorPayload(Owner->GetVelocity()));
	}
}
