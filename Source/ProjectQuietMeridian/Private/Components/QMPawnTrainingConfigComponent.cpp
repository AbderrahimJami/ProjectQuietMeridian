// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/QMPawnTrainingConfigComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/Pawn.h"

UQMPawnTrainingConfigComponent::UQMPawnTrainingConfigComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UQMPawnTrainingConfigComponent::ApplyPawnSettings(const FQMPawnWorldSettings& InSettings)
{
	AActor* Owner = GetOwner();
	APawn* Pawn = Cast<APawn>(Owner);
	if (!Owner || !Pawn)
	{
		return;
	}

	if (InSettings.bApplyTransform)
	{
		Owner->SetActorTransform(InSettings.Transform.ToTransform());
	}

	if (InSettings.bApplyMaxSpeed)
	{
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = InSettings.MaxSpeed;
				Movement->MaxFlySpeed = InSettings.MaxSpeed;
			}
		}
		else if (UFloatingPawnMovement* FloatingMovement = Pawn->FindComponentByClass<UFloatingPawnMovement>())
		{
			FloatingMovement->MaxSpeed = InSettings.MaxSpeed;
		}
	}

	OnPawnSettingsApplied.Broadcast(InSettings);
}
