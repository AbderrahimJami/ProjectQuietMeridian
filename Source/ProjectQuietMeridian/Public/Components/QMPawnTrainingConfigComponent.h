// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Data/QMTrainingSettingsTypes.h"
#include "QMPawnTrainingConfigComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQMPawnSettingsAppliedSignature, const FQMPawnWorldSettings&, AppliedSettings);

UCLASS(ClassGroup = (Training), meta = (BlueprintSpawnableComponent))
class PROJECTQUIETMERIDIAN_API UQMPawnTrainingConfigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQMPawnTrainingConfigComponent();

	UFUNCTION(BlueprintCallable, Category = "QM|Training")
	void ApplyPawnSettings(const FQMPawnWorldSettings& InSettings);

	UPROPERTY(BlueprintAssignable, Category = "QM|Training")
	FQMPawnSettingsAppliedSignature OnPawnSettingsApplied;
};
