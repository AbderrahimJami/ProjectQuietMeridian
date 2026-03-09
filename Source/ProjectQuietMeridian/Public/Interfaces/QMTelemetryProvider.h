// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "QMTelemetryProvider.generated.h"

class FJsonObject;

UINTERFACE(MinimalAPI)
class UQMTelemetryProvider : public UInterface
{
	GENERATED_BODY()
};

class PROJECTQUIETMERIDIAN_API IQMTelemetryProvider
{
	GENERATED_BODY()

public:
	virtual void GatherTelemetry(TSharedRef<FJsonObject> OutTelemetry) const
	{
	}
};
