// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Interfaces/QMTelemetryProvider.h"
#include "QMSimpleTelemetryProviderComponent.generated.h"

class FJsonObject;

UCLASS(ClassGroup = (Training), meta = (BlueprintSpawnableComponent))
class PROJECTQUIETMERIDIAN_API UQMSimpleTelemetryProviderComponent : public UActorComponent, public IQMTelemetryProvider
{
	GENERATED_BODY()

public:
	UQMSimpleTelemetryProviderComponent();

	virtual void GatherTelemetry(TSharedRef<FJsonObject> OutTelemetry) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QM|Telemetry")
	bool bIncludeOwnerTransform = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QM|Telemetry")
	bool bIncludeOwnerVelocity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QM|Telemetry")
	bool bIncludeWorldTimeSeconds = true;
};
