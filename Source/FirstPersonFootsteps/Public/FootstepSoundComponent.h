// Copyright (c) 2023 Hitbox Initiative. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FootstepSoundSet.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FootstepSoundComponent.generated.h"

UENUM(BlueprintType)
enum class EFootstepMovementType : uint8
{
	Stopped,
	Walking,
	CrouchWalking,
	Flying
};


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FIRSTPERSONFOOTSTEPS_API UFootstepSoundComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMovementEvent);
	
public:
	UFootstepSoundComponent();
	
	/**
	 * Delegate broadcast on when a footstep is played.
	 */
	UPROPERTY(BlueprintAssignable)
	FMovementEvent OnFootstep;

	/**
	 * Delegate broadcast on when the actor jumps.
	 */
	UPROPERTY(BlueprintAssignable)
	FMovementEvent OnJump;

	/**
	 * Delegate broadcast on when the actor lands.
	 */
	UPROPERTY(BlueprintAssignable)
	FMovementEvent OnLand;

	/**
	* Delegate broadcast on when the actor starts moving.
	*/
	UPROPERTY(BlueprintAssignable)
	FMovementEvent OnStartMoving;

	/**
	* Delegate broadcast on when the actor stops moving.
	*/
	UPROPERTY(BlueprintAssignable)
	FMovementEvent OnStopMoving;

	/**
	 * A float curve that scales footstep volume (0 - 1) based on movement speed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> VolumeSpeedCurve;

	/**
	 * A float curve that scales footstep interval (centimeters) based on movement speed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> IntervalSpeedCurve;

	/**
	 * A float curve that scales landing volume (0 - 1) based on earlier movement speed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> LandingVolumeSpeedCurve;

	/**
	 * Sound sets for different surfaces.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<UFootstepSoundSet>> SoundSets;

	/**
	 * Distance which the character can be above ground and still be considered on ground.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ActorOnGroundDistance = 10.f;

	/**
	 * Minimum interval in seconds between footsteps. Will cancel the next footstep in this time period.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MinimumSoundInterval = 0.12f;

	/**
	 * Footstep sound multiplier when crouched.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CrouchSoundMultiplier = 0.5f;

	/**
	 * Collision channel used to detect surface below for footsteps.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TEnumAsByte<ECollisionChannel> CollisionChannelForSurfaceDetection = ECC_Camera;

	/**
	 * Register that a jump has happened.
	 */
	UFUNCTION(BlueprintCallable)
	void RegisterJump() { OnJump_Native(); }
	
	// Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;
	// End UActorComponent Interface.

private:
	void PlaySound(USoundBase* Sound, float VolumeOverride = 1.f);
	bool VerifyCharacter() const;
	void ReadMovementComponent();
	void UpdateSurface();
	void UpdateSpeed();
	void ResolveDependencies();
	void PlayFootsteps();
	UFootstepSoundSet* PickSoundSet();
	float CalculateFootstepVolume() const;
	float CalculateLandingVolume() const;

	void OnJump_Native();
	void OnLand_Native();
	void OnStartMoving_Native();
	void OnStopMoving_Native();

	float StepDistanceRemaining = 20.f;
	EFootstepMovementType CachedMovementType = EFootstepMovementType::Stopped;
	EPhysicalSurface CachedPhysicalSurface = SurfaceType_Default;
	float SurfaceAge = 0.f;
	FVector LastStepLocation = FVector::ZeroVector;
	float CachedSpeed = 0.f;
	float TimeSinceLastFootstep = 0.f;

	TWeakObjectPtr<ACharacter> CachedOwner;
	TWeakObjectPtr<UCharacterMovementComponent> CachedMovementComponent;
	
};
