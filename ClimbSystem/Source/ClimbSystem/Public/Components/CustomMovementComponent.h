// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

DECLARE_DELEGATE(FOnEnterClimbState)
DECLARE_DELEGATE(FOnExitClimbState)


class UAnimMontage;
class UAnimInstance;
class AClimbingSystemCharacter;

UENUM(BlueprintType)
namespace ECustomMovementMode
{
	enum Type
	{
		MOVE_Climb UMETA(DisplayName = "Climb Mode")
	};
}
/**
 * 
 */
UCLASS()
class CLIMBSYSTEM_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	FOnEnterClimbState OnEnterClimbStateDelegate;
	FOnExitClimbState OnExitClimbStateDelegate;


private:
	ACharacter* MyChar;
	UCharacterMovementComponent* MyMove;
#pragma region OverridenFunctions


protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode = 0) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;

#pragma endregion

private:

#pragma region Helpers
	bool IsHitWalkableSurface(const FHitResult& Hit) const;
	

#pragma endregion

#pragma region ClimbTraces

	TArray<FHitResult> DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape = false, bool bDrawPersistantShapes = false);
	FHitResult DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape = false, bool bDrawPersistantShapes = false);

#pragma endregion

#pragma region ClimbCore

	bool TraceClimbableSurfaces();
	FHitResult TraceFromEyeHeight(float TraceDistance, float TraceStartOffset = 0.f);
	bool CanStartClimbing();

	bool CanClimbDownLedge();
	void StartClimbing();
	void StopClimbing();

	void PhysClimb(float deltaTime, int32 Iterations);

	void ProcessClimbableSurfaceInfo();

	bool CheckShouldStopClimbing();

	bool CheckHasReachedFloor();


	FQuat GetClimbRotation(float  DeltaTime);

	void SnapMovementToClimbableSurfaces(float DeltaTime);

	bool CheckHasReachedLedge();

	bool HasEnoughTopSurfaceDepthAndWidth(
		const FVector& TopSurfacePoint,
		const FVector& ForwardDir,
		float WalkableZ
	);

	bool CanStandAtLocation(const FVector& StandLocation) const;


	void TryStartVaulting();

	bool CanStartVaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition);

	void PlayClimbMontage(UAnimMontage* MontageToPlay);

	UFUNCTION()
	void OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition);

	void HandleHopUp();

	bool CheckCanHopUp(FVector& OutHopUpTargetPosition);

	void HandleHopDown();

	bool CheckCanHopDown(FVector& OutHopDownTargetPosition);

#pragma endregion



#pragma region ClimbCore Variables

	TArray<FHitResult> ClimbableSurfaceTracedResults; // i should clear after climb should i limit the size?

	FVector CurrentClimbableSurfaceLocation;

	FVector CurrentClimbableSurfaceNormal;

	FVector SmoothedClimbNormal = FVector::ZeroVector;
	bool bHasSmoothedClimbNormal = false;

	UPROPERTY(EditAnywhere, Category = "Climbing|Smoothing")
	float NormalSmoothSpeed = 18.f; 

	UPROPERTY()
	UAnimInstance* OwningPlayerAnimInstance;

	UPROPERTY()
	AClimbingSystemCharacter* OwningPlayerCharacter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopUpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopDownMontage;
#pragma endregion



#pragma region ClimbBPVariables


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	TArray<TEnumAsByte<EObjectTypeQuery> >ClimbableSurfaceTraceTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceRadius = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceHalfHeight = 72.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	float MaxBreakClimbDeceleration = 400.0f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	float MaxClimbSpeed = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	float MaxClimbAcceleration = 300.0f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	float ClimbDownWalkableSurfaceTraceOffset = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	
	float ClimbDownLedgeTraceOffset = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* IdleToClimbMontage;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbToTopMontage;



	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement :Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbDownLedgeMontage;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* VaultMontage;

	// how far onto the top surface we need to be able to stand
	UPROPERTY(EditAnywhere, Category = "Climb|Ledge")
	float ClimbUpMinDepth = 60.f;

	// how many forward samples to take across the top
	UPROPERTY(EditAnywhere, Category = "Climb|Ledge")
	int32 ClimbUpDepthSamples = 3;

	UPROPERTY(EditAnywhere, Category = "Climb|Ledge")
	int32 ClimbUpWidthSamples = 3;   // 3 = left/center/right

	UPROPERTY(EditAnywhere, Category = "Climb|Ledge")
	float ClimbUpWidthSpacing = 20.f; // spacing between lanes

	// spacing between samples (total checked depth = Samples * Spacing)
	UPROPERTY(EditAnywhere, Category = "Climb|Ledge")
	float ClimbUpSampleSpacing = 25.f;

	// how far down we trace to find the top surface
	UPROPERTY(EditAnywhere, Category = "Climb|Ledge")
	float ClimbUpTopDownTraceDist = 150.f;

	// optional: how much vertical clearance above the top surface we require
	UPROPERTY(EditAnywhere, Category = "Climb|")
	float ClimbUpHeadClearance = 120.f;

#pragma endregion


public:
	void ToggleClimbing(bool bEnableClimb);
	void RequestHopping();
	
	bool IsClimbing() const;
	FORCEINLINE FVector GetClimbableSurfaceNormal() const { return CurrentClimbableSurfaceNormal; }

	FVector GetUnrotatedClimbVelocity() const;
};

