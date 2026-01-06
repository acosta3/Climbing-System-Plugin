// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CustomMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ClimbingSystemCharacter.h"

#include "DebugHelper.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ClimbingSystemCharacter.h"
#include "MotionWarpingComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"





void UCustomMovementComponent::BeginPlay()
{
	Super::BeginPlay();



	OwningPlayerAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
	if (OwningPlayerAnimInstance)
	{
		OwningPlayerAnimInstance->OnMontageEnded.AddDynamic(this, &UCustomMovementComponent::OnClimbMontageEnded);
		OwningPlayerAnimInstance->OnMontageBlendingOut.AddDynamic(this, &UCustomMovementComponent::OnClimbMontageEnded);
	}

	MyChar = Cast<ACharacter>(GetOwner()); // if you're in a component
	if (MyChar)
	{
		MyMove = MyChar->GetCharacterMovement();
	}

	OwningPlayerCharacter = Cast<AClimbingSystemCharacter>(CharacterOwner);

}


void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	

}




void UCustomMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (IsClimbing())
	{
		bOrientRotationToMovement = false;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(48.f);

		OnEnterClimbStateDelegate.ExecuteIfBound();

	}
	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
	{
		bOrientRotationToMovement = true;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);

		const FRotator DirtyRotation = UpdatedComponent->GetComponentRotation();
		const FRotator CleanStandRotation = FRotator(0.f, DirtyRotation.Yaw, 0.f);

		UpdatedComponent->SetRelativeRotation(CleanStandRotation);	




		StopMovementImmediately();
		OnExitClimbStateDelegate.ExecuteIfBound();
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UCustomMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{ 
	if (IsClimbing())
	{
		PhysClimb(deltaTime, Iterations);
	}
	Super::PhysCustom(deltaTime, Iterations);
}

float UCustomMovementComponent::GetMaxSpeed() const
{
	if (IsClimbing())
	{
		return MaxClimbSpeed;
	}
	else
	{
		return Super::GetMaxSpeed();
	}
}

float UCustomMovementComponent::GetMaxAcceleration() const
{
	if (IsClimbing())
	{
		return MaxClimbAcceleration;
	}
	else
	{
		return Super::GetMaxAcceleration();
	}
}

FVector UCustomMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const
{
	const bool bIsPlayingRMMontage =
	IsFalling() && OwningPlayerAnimInstance&& OwningPlayerAnimInstance->IsAnyMontagePlaying();

	if(bIsPlayingRMMontage)
	{
		return RootMotionVelocity;
	}
	else 
	{
		return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);
	}


}

bool UCustomMovementComponent::IsHitWalkableSurface(const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit) return false;

	// Uses WalkableFloorAngle / WalkableFloorZ rules from CharacterMovement
	return IsWalkable(Hit);
}


#pragma region ClimbTraces


TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
	TArray<FHitResult> OutCapsuleTraceHitResults;

	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;

	if (bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;
		if (bDrawPersistantShapes)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
	}


	UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		this,
		Start,
		End,
		ClimbCapsuleTraceRadius,
		ClimbCapsuleTraceHalfHeight,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		DebugTraceType,
		OutCapsuleTraceHitResults,
		false
	);

	return OutCapsuleTraceHitResults;
}




FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
	FHitResult OutHit;
	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;

	if (bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;
		if (bDrawPersistantShapes)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
	}


	UKismetSystemLibrary::LineTraceSingleForObjects(
		this,
		Start,
		End,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		DebugTraceType,
		OutHit,
		false
	);
	return OutHit;
}

#pragma endregion


#pragma region ClimbCore
void UCustomMovementComponent::ToggleClimbing(bool bEnableClimb)
{
	if(bEnableClimb)
	{
		if(CanStartClimbing())
		{
			// enter climb state
			
			PlayClimbMontage(IdleToClimbMontage);
			
		}
		else if (CanClimbDownLedge())
		{
			PlayClimbMontage(ClimbDownLedgeMontage);
		}
		else
		{
			TryStartVaulting();
		}
		
	}
	if (!bEnableClimb)
	{
		// stop climbing
		StopClimbing();
	}

}

void UCustomMovementComponent::RequestHopping()
{
	const FVector UnrotatedLastInputVector =
		UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), GetLastInputVector());

	const float DotResult =
		FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), FVector::UpVector);

	

	if (DotResult >= 0.9f)
	{
		
		HandleHopUp();
	}
	else if (DotResult <= -0.9f)
	{
		
		HandleHopDown();
	}
	else
	{
		
	}
}

void UCustomMovementComponent::HandleHopUp()
{
	FVector HopUpTargetPoint;

	if (CheckCanHopUp(HopUpTargetPoint))
	{
		SetMotionWarpTarget(FName("HopUpTargetPoint"), HopUpTargetPoint);

		PlayClimbMontage(HopUpMontage);
	}
}

bool UCustomMovementComponent::CheckCanHopUp(FVector& OutHopUpTargetPosition)
{
	FHitResult HopUpHit = TraceFromEyeHeight(100.f, -20.f);
	FHitResult SaftyLedgeHit = TraceFromEyeHeight(100.f, 150.f);

	if (HopUpHit.bBlockingHit && SaftyLedgeHit.bBlockingHit)
	{
		OutHopUpTargetPosition = HopUpHit.ImpactPoint;

		return true;
	}

	return false;
}

void UCustomMovementComponent::HandleHopDown()
{
	FVector HopDownTargetPoint;

	if (CheckCanHopDown(HopDownTargetPoint))
	{
		SetMotionWarpTarget(FName("HopDownTargetPoint"), HopDownTargetPoint);

		PlayClimbMontage(HopDownMontage);
	}
}

bool UCustomMovementComponent::CheckCanHopDown(FVector& OutHopDownTargetPosition)
{
	FHitResult HopDownHit = TraceFromEyeHeight(100.f, -300.f);

	if (HopDownHit.bBlockingHit)
	{
		OutHopDownTargetPosition = HopDownHit.ImpactPoint;

		return true;
	}

	return false;
}



bool UCustomMovementComponent::CanStartClimbing()
{
	if(IsFalling()) // might have to change
	{
		return false;
	}

	if(!TraceClimbableSurfaces())
	{
		return false;
	}

	if(!TraceFromEyeHeight(100.f).bBlockingHit)
	{
		return false;
	}

	return true;
}
bool UCustomMovementComponent::CanClimbDownLedge()
{
	if (IsFalling()) return false;

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector DownVector = -UpdatedComponent->GetUpVector();

	const FVector WalkableSurfaceTraceStart = ComponentLocation + ComponentForward * ClimbDownWalkableSurfaceTraceOffset;
	const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

	FHitResult WalkableSurfaceHit = DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, false);

	const FVector LedgeTraceStart = WalkableSurfaceHit.TraceStart + ComponentForward * ClimbDownLedgeTraceOffset;
	const FVector LedgeTraceEnd = LedgeTraceStart + DownVector * 200.f;

	FHitResult LedgeTraceHit = DoLineTraceSingleByObject(LedgeTraceStart, LedgeTraceEnd, false);

	if (WalkableSurfaceHit.bBlockingHit && !LedgeTraceHit.bBlockingHit)
	{
		return true;
	}

	return false;
}



void UCustomMovementComponent::StartClimbing()
{
	SetMovementMode(MOVE_Custom, ECustomMovementMode::MOVE_Climb);
}

void UCustomMovementComponent::StopClimbing()
{
	
	SetMovementMode(MOVE_Falling);
	ClimbableSurfaceTracedResults.Reset();
	CurrentClimbableSurfaceLocation = FVector::ZeroVector;
	CurrentClimbableSurfaceNormal = FVector::ZeroVector;
	SmoothedClimbNormal = FVector::ZeroVector;
	bHasSmoothedClimbNormal = false;

}






void UCustomMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
	/*Called every frame once in climbing state*/
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	
	

	/*Process all the climbable surfaces info*/
	TraceClimbableSurfaces();

	ProcessClimbableSurfaceInfo();


	if (CheckShouldStopClimbing() || CheckHasReachedFloor())
	{
		StopClimbing();
		return;
	}

	/*check if we should stop climbing*/
	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		// define the max climb speed and acceleration
		CalcVelocity(deltaTime, 0.f, true, MaxBreakClimbDeceleration);
	}

	ApplyRootMotionToVelocity(deltaTime);

	

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);

	// handle climb rotation
	SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(deltaTime), true, Hit);

	if (Hit.Time < 1.f)
	{
		//adjust and try again
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
		
	}

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
	/*snap movement to climable surfaces*/
	SnapMovementToClimbableSurfaces(deltaTime);

	if (CheckHasReachedLedge())
	{
		
		PlayClimbMontage(ClimbToTopMontage);
	}
}

void UCustomMovementComponent::ProcessClimbableSurfaceInfo()
{
	FVector AvgLoc = FVector::ZeroVector;
	FVector AvgNormal = FVector::ZeroVector;

	if (ClimbableSurfaceTracedResults.IsEmpty())
		return;

	for (const FHitResult& H : ClimbableSurfaceTracedResults)
	{
		AvgLoc += H.ImpactPoint;
		AvgNormal += H.ImpactNormal;
	}

	AvgLoc /= ClimbableSurfaceTracedResults.Num();
	AvgNormal = AvgNormal.GetSafeNormal();

	// ---- Smooth the normal ----
	if (!bHasSmoothedClimbNormal)
	{
		SmoothedClimbNormal = AvgNormal;
		bHasSmoothedClimbNormal = true;
	}
	else
	{
		// Smooth in a rotation-safe way: interp between directions then renormalize
		const float Alpha = 1.f - FMath::Exp(-NormalSmoothSpeed * GetWorld()->GetDeltaSeconds());
		SmoothedClimbNormal = (SmoothedClimbNormal * (1.f - Alpha) + AvgNormal * Alpha).GetSafeNormal();
	}

	CurrentClimbableSurfaceLocation = AvgLoc;
	CurrentClimbableSurfaceNormal = SmoothedClimbNormal;
}


bool UCustomMovementComponent::CheckShouldStopClimbing()
{
	if(ClimbableSurfaceTracedResults.IsEmpty())
	{
		return true;
	}
	const float DotResult = FVector::DotProduct(CurrentClimbableSurfaceNormal, FVector::UpVector);
	const float DegreeDiff = FMath::RadiansToDegrees(FMath::Acos(DotResult));

	if (DegreeDiff <= 20) // can be adjusted for our geometry
	{
		return true;
	}
	return false;
}


bool UCustomMovementComponent::CheckHasReachedFloor()
{
	if (!UpdatedComponent || !MyMove) return false;

	// must be moving down while climbing
	if (GetUnrotatedClimbVelocity().Z >= -10.f) return false;

	const float WalkableZ = MyMove->GetWalkableFloorZ();

	const FVector Up = FVector::UpVector;
	const FVector DownVector = -UpdatedComponent->GetUpVector();

	const FVector Start = UpdatedComponent->GetComponentLocation() + DownVector * 10.f;
	const FVector End = Start + DownVector * 80.f; // IMPORTANT: real distance

	TArray<FHitResult> PossibleFloorHits = DoCapsuleTraceMultiByObject(Start, End, false);
	if (PossibleFloorHits.IsEmpty()) return false;

	// Pick best walkable floor hit (highest Z that is walkable)
	FHitResult BestFloorHit;
	bool bFound = false;

	for (const FHitResult& Hit : PossibleFloorHits)
	{
		if (!Hit.bBlockingHit) continue;

		const float DotUp = FVector::DotProduct(Hit.ImpactNormal, Up);
		if (DotUp < WalkableZ) continue; // not walkable, skip

		if (!bFound || Hit.ImpactPoint.Z > BestFloorHit.ImpactPoint.Z)
		{
			BestFloorHit = Hit;
			bFound = true;
		}
	}

	if (!bFound) return false;

	// Depth + width check around the floor point
	const FVector ForwardDir = UpdatedComponent->GetForwardVector();


	// use minus since we want to traces backwards from our forward dir
	return HasEnoughTopSurfaceDepthAndWidth(BestFloorHit.ImpactPoint, -ForwardDir, WalkableZ);
}


FQuat UCustomMovementComponent::GetClimbRotation(float DeltaTime)
{
	const FQuat CurrentQuat = UpdatedComponent->GetComponentQuat();

	if(HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		return CurrentQuat;
	}
	const FQuat TargetQuat = FRotationMatrix::MakeFromX(-CurrentClimbableSurfaceNormal).ToQuat();

	return FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, 5.f);
	
}



void UCustomMovementComponent::SnapMovementToClimbableSurfaces(float DeltaTime)
{
	const FVector Loc = UpdatedComponent->GetComponentLocation();

	const FVector IntoWall = -CurrentClimbableSurfaceNormal;
	if (IntoWall.IsNearlyZero()) return;

	const float TraceLen = 120.f;
	const FVector Start = Loc;
	const FVector End = Loc + IntoWall * TraceLen;

	FHitResult WallHit = DoLineTraceSingleByObject(Start, End, false);

	if (!WallHit.bBlockingHit) return;

	const float TargetDist = 43.5f; // tune for your capsule and animations
	const float CurrentDist = WallHit.Distance;

	const float Error = (TargetDist - CurrentDist);

	


	FVector Correction = -IntoWall * Error; // move outward/inward along normal

	//const float MaxSnapPerTick = 10.f; // doesnt scale with different hardware

	const float MaxSnapThisFrame = MaxClimbSpeed * DeltaTime;

	Correction = Correction.GetClampedToMaxSize(MaxSnapThisFrame);
	

	FHitResult Hit;
	SafeMoveUpdatedComponent(Correction, UpdatedComponent->GetComponentQuat(), true, Hit);

	
}

bool UCustomMovementComponent::CheckHasReachedLedge()
{
	FHitResult LedgeHitResult = TraceFromEyeHeight(100.f, 50.f);

	// We only proceed if there is NO wall directly in front (space to climb up)
	if (LedgeHitResult.bBlockingHit)
	{
		return false;
	}

	const FVector ForwardDir = UpdatedComponent->GetForwardVector();
	const FVector Up = FVector::UpVector;
	const FVector Down = -Up;

	// Find the top surface by tracing down from the forward edge point
	const FVector WalkableSurfaceTraceStart = LedgeHitResult.TraceEnd + Up * 60.f;
	const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + Down * 150.f;

	FHitResult WalkableSurfaceHitResult =
		DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, false);

	if (!WalkableSurfaceHitResult.bBlockingHit)
	{
		return false;
	}

	// Must be moving upward enough (your original rule)
	if (GetUnrotatedClimbVelocity().Z <= 10.f)
	{
		return false;
	}

	if (!MyMove)
	{
		return false;
	}

	const float WalkableZ = MyMove->GetWalkableFloorZ();
	const float DotUp = FVector::DotProduct(WalkableSurfaceHitResult.ImpactNormal, Up);
	if (DotUp < WalkableZ)
	{
		return false;
	}

	
	const FVector TopSurfacePoint = WalkableSurfaceHitResult.ImpactPoint;

	if (!HasEnoughTopSurfaceDepthAndWidth(TopSurfacePoint, ForwardDir, WalkableZ))
	{
		return false;
	}

	
	const FVector StandSpot = TopSurfacePoint + ForwardDir * 50.f + Up * 90.f;
	if (!CanStandAtLocation(StandSpot))
	{
		
		return false;
	}

	return true;
}


bool UCustomMovementComponent::HasEnoughTopSurfaceDepthAndWidth(
	const FVector& TopSurfacePoint,
	const FVector& ForwardDir,
	float WalkableZ
) 
{
	const FVector Up = FVector::UpVector;
	const FVector Down = -Up;

	// Right direction for width lanes
	FVector RightDir = FVector::CrossProduct(Up, ForwardDir).GetSafeNormal();
	if (RightDir.IsNearlyZero())
	{
		RightDir = UpdatedComponent->GetRightVector();
	}

	const float StartAbove = 60.f;
	const float MaxHeightDelta = 25.f;

	float ReferenceZ = 0.f;
	bool bHasReference = false;

	// width lanes centered around 0
	const int32 HalfW = ClimbUpWidthSamples / 2;

	for (int32 d = 0; d < ClimbUpDepthSamples; ++d)
	{
		const float ForwardOffset = (d + 1) * ClimbUpSampleSpacing;

		for (int32 w = -HalfW; w <= HalfW; ++w)
		{
			const float SideOffset = w * ClimbUpWidthSpacing;

			const FVector SampleBase =
				TopSurfacePoint +
				ForwardDir * ForwardOffset +
				RightDir * SideOffset;

			const FVector Start = SampleBase + Up * StartAbove;
			const FVector End = SampleBase + Down * ClimbUpTopDownTraceDist;

			const FHitResult Hit = DoLineTraceSingleByObject(Start, End, false);

			if (!Hit.bBlockingHit)
			{
				return false; // missing floor somewhere in the grid
			}

			const float DotUp = FVector::DotProduct(Hit.ImpactNormal, Up);
			if (DotUp < WalkableZ)
			{
				
				return false; // not walkable at this sample
			}

			if (!bHasReference)
			{
				ReferenceZ = Hit.ImpactPoint.Z;
				bHasReference = true;
			}
			else
			{
				if (FMath::Abs(Hit.ImpactPoint.Z - ReferenceZ) > MaxHeightDelta)
				{
					return false; // too uneven
				}
			}
		}
	}

	// enforce minimum forward depth checked
	const float TotalCheckedDepth = ClimbUpDepthSamples * ClimbUpSampleSpacing;
	return TotalCheckedDepth >= ClimbUpMinDepth;
}


bool UCustomMovementComponent::CanStandAtLocation(const FVector& StandLocation) const
{
	if (!CharacterOwner) return false;

	UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	if (!Capsule) return false;

	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClimbUpStandCheck), false, CharacterOwner);

	// Overlap the capsule at the final stand location
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	const bool bOverlaps = GetWorld()->OverlapBlockingTestByChannel(
		StandLocation,
		FQuat::Identity,
		ECC_Pawn,
		Shape,
		Params
	);

	// if it overlaps something blocking, we cannot stand there
	return !bOverlaps;
}



void UCustomMovementComponent::TryStartVaulting()
{
	FVector VaultStartPosition;
	FVector VaultLandPosition;

	if (CanStartVaulting(VaultStartPosition, VaultLandPosition))
	{
	
		SetMotionWarpTarget(FName("VaultStartPoint"), VaultStartPosition);
		SetMotionWarpTarget(FName("VaultEndPoint"), VaultLandPosition);

		StartClimbing();
		PlayClimbMontage(VaultMontage);
	}
}

bool UCustomMovementComponent::CanStartVaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition)
{
	if (IsFalling()) return false;

	OutVaultStartPosition = FVector::ZeroVector;
	OutVaultLandPosition = FVector::ZeroVector;

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector UpVector = UpdatedComponent->GetUpVector();
	const FVector DownVector = -UpdatedComponent->GetUpVector();

	for (int32 i = 0; i < 5; i++)
	{
		const FVector Start = ComponentLocation + UpVector * 100.f +
			ComponentForward * 80.f * (i + 1);

		const FVector End = Start + DownVector * 150.f * (i + 1); // you can tune value for vaulting

		FHitResult VaultTraceHit = DoLineTraceSingleByObject(Start, End);

		if (i == 0 && VaultTraceHit.bBlockingHit && IsHitWalkableSurface(VaultTraceHit))
		{
			OutVaultStartPosition = VaultTraceHit.ImpactPoint;
		}

		if (i == 2 && VaultTraceHit.bBlockingHit && IsHitWalkableSurface(VaultTraceHit))
		{
			OutVaultLandPosition = VaultTraceHit.ImpactPoint;
		}
	}

	if (OutVaultStartPosition != FVector::ZeroVector && OutVaultLandPosition != FVector::ZeroVector)
	{
		return true;
	}
	else
	{
		return false;
	}

}


void UCustomMovementComponent::PlayClimbMontage(UAnimMontage* MontageToPlay)
{
	if(!MontageToPlay || !OwningPlayerAnimInstance || OwningPlayerAnimInstance->IsAnyMontagePlaying())
	{
		return;
	}

	OwningPlayerAnimInstance->Montage_Play(MontageToPlay);
	
}

void UCustomMovementComponent::OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	
	if(Montage == IdleToClimbMontage || Montage == ClimbDownLedgeMontage)
	{
		StartClimbing();
		StopMovementImmediately();
	}
	if (Montage == ClimbToTopMontage || Montage == VaultMontage)
	{
		SetMovementMode(MOVE_Walking);
	}
}

void UCustomMovementComponent::SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition)
{
	if (!OwningPlayerCharacter) return;

	OwningPlayerCharacter->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocation(
		InWarpTargetName,
		InTargetPosition
	);
}




bool UCustomMovementComponent::IsClimbing() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::MOVE_Climb;
	
}

FVector UCustomMovementComponent::GetUnrotatedClimbVelocity() const
{
	return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), Velocity);
	
}



bool UCustomMovementComponent::TraceClimbableSurfaces()
{
	


	const FVector Loc = UpdatedComponent->GetComponentLocation();

	const FVector IntoWall =
		(CurrentClimbableSurfaceNormal.IsNearlyZero())
		? UpdatedComponent->GetForwardVector()
		: -CurrentClimbableSurfaceNormal;

	const FVector Start = Loc + IntoWall * 20.f;
	const FVector End = Start + IntoWall * 30.f;   // important, not 10

	ClimbableSurfaceTracedResults = DoCapsuleTraceMultiByObject(Start, End, false);
	
	return !ClimbableSurfaceTracedResults.IsEmpty();


}

FHitResult UCustomMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset)
{
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeighOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector Start = ComponentLocation + EyeHeighOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

	return DoLineTraceSingleByObject(Start, End,false);

}

#pragma endregion



