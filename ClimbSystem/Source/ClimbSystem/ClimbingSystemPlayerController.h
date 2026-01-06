// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ClimbingSystemPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class CLIMBSYSTEM_API  AClimbingSystemPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> ClimbMappingContexts;


	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;




	UPROPERTY(EditDefaultsOnly, Category = "Input|Input Mappings")
	int32 ClimbPriority = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Input Mappings")
	int32 DefaultPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Input Mappings")
	int32 MobileExcludedPriority = 0;




	/** Gameplay initialization */
	virtual void BeginPlay() override;


	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

public:
	void SetInputMode_Climb(bool bClimbing);

private:
	

	


};
