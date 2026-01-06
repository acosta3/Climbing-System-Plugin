// Copyright Epic Games, Inc. All Rights Reserved.


#include "ClimbingSystemPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "ClimbSystem.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AClimbingSystemPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogTemp, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AClimbingSystemPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController()) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem) return;

	for (UInputMappingContext* C : DefaultMappingContexts)
	{
		if (C) Subsystem->AddMappingContext(C, DefaultPriority);
	}

	if (!SVirtualJoystick::ShouldDisplayTouchInterface())
	{
		for (UInputMappingContext* C : MobileExcludedMappingContexts)
		{
			if (C) Subsystem->AddMappingContext(C, MobileExcludedPriority);
		}
	}
}


void AClimbingSystemPlayerController::SetInputMode_Climb(bool bClimbing)
{
	if (!IsLocalPlayerController()) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem) return;

	if (ClimbMappingContexts.IsEmpty()) 
	{
		return;
	}

	if (bClimbing)
	{

		
		for (UInputMappingContext* C : ClimbMappingContexts)
		{
			if (C) Subsystem->AddMappingContext(C, ClimbPriority);
		}
	}
	else
	{
		// climb off, base back on
		for (UInputMappingContext* C : ClimbMappingContexts)
		{
			if (C) Subsystem->RemoveMappingContext(C);
		}

	}
}