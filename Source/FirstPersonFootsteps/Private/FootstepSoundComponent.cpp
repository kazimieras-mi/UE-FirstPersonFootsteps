// Copyright (c) 2023 Hitbox Initiative. All rights reserved.

#include "FootstepSoundComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

UFootstepSoundComponent::UFootstepSoundComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFootstepSoundComponent::BeginPlay()
{
	Super::BeginPlay();	

	if(!ensureAlwaysMsgf(VerifyCharacter(), TEXT("Footstep sound component can only be used on characters.")))
	{
		DestroyComponent();
		return;
	}

	ResolveDependencies();

	LastStepLocation = CachedOwner->GetActorLocation();
}

void UFootstepSoundComponent::PlaySound(USoundBase* Sound, const float VolumeOverride)
{
	if(!ensureAlwaysMsgf(IsValid(Sound), TEXT("Sound not valid.")))
	{
		return;
	}

	if(TimeSinceLastFootstep >= MinimumSoundInterval)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, CachedOwner->GetActorLocation(), FRotator::ZeroRotator, VolumeOverride);
		TimeSinceLastFootstep = 0.f;
		OnFootstep.Broadcast();
	}
}

bool UFootstepSoundComponent::VerifyCharacter() const
{
	if(dynamic_cast<ACharacter*>(GetOwner()))
	{
		return true;
	}
	return false;
}

void UFootstepSoundComponent::ReadMovementComponent()
{
	EFootstepMovementType NewMovementType;

	if(CachedMovementComponent->IsMovingOnGround())
	{
		if(CachedSpeed < UE_SMALL_NUMBER)
		{
			NewMovementType = EFootstepMovementType::Stopped;
		} else
		{
			NewMovementType = CachedMovementComponent->IsCrouching() ? EFootstepMovementType::CrouchWalking : EFootstepMovementType::Walking;
		}
	} else
	{
		NewMovementType = EFootstepMovementType::Flying;
	}

	if(CachedMovementType != NewMovementType)
	{
		if(CachedMovementType == EFootstepMovementType::Flying)
		{
			OnLand_Native();
		} else
		{
			const bool bWasMoving = CachedMovementType == EFootstepMovementType::Walking || CachedMovementType == EFootstepMovementType::CrouchWalking;
			const bool bIsMoving = NewMovementType == EFootstepMovementType::Walking || NewMovementType == EFootstepMovementType::CrouchWalking;

			if(bWasMoving && !bIsMoving)
			{
				OnStopMoving_Native();
			} else if (!bWasMoving && bIsMoving)
			{
				OnStartMoving_Native();
			}
		}
	}
	CachedMovementType = NewMovementType;
}

void UFootstepSoundComponent::UpdateSurface()
{
	const UWorld* World = GetWorld();
	if(!ensureAlwaysMsgf(IsValid(World), TEXT("World not valid.")))
	{
		return;
	}

	struct FFootstepQueryParams : FCollisionQueryParams
	{
		FFootstepQueryParams ()
		{
			TraceTag = TEXT("Footstep");
			bTraceComplex = false;
			bReturnPhysicalMaterial = true;
		}

		void SetIgnoredActor(const AActor* Actor)
		{
			ClearIgnoredActors();
			if(IsValid(Actor))
			{
				AddIgnoredActor(Actor);
			}
		}
	};
	static FFootstepQueryParams QueryParams;
	QueryParams.SetIgnoredActor(GetOwner());

	const FVector Start = CachedOwner->GetActorLocation();
	const FVector End =  Start - FVector(0.f, 0.f, CachedOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + ActorOnGroundDistance);
	
	FHitResult OutHit;
	if(World->LineTraceSingleByChannel(OutHit, Start, End, CollisionChannelForSurfaceDetection, QueryParams))
	{
		if(const UPhysicalMaterial* PhysicalMaterial = OutHit.PhysMaterial.Get())
		{
			CachedPhysicalSurface = PhysicalMaterial->SurfaceType;
		}
	}
}

void UFootstepSoundComponent::UpdateSpeed()
{
	CachedSpeed = CachedOwner->GetVelocity().Size();
}

void UFootstepSoundComponent::ResolveDependencies()
{
	CachedOwner = dynamic_cast<ACharacter*>(GetOwner());
	if(ensureAlwaysMsgf(CachedOwner.IsValid(), TEXT("Owner is not valid.")))
	{
		CachedMovementComponent = dynamic_cast<UCharacterMovementComponent*>(CachedOwner.Get()->GetMovementComponent());
	}
}

void UFootstepSoundComponent::PlayFootsteps()
{
	if(CachedMovementType != EFootstepMovementType::Walking && CachedMovementType != EFootstepMovementType::CrouchWalking)
	{
		return;
	}

	const FVector OwnerLocation = CachedOwner->GetActorLocation();
	const float Distance = (OwnerLocation - LastStepLocation).Size();
	LastStepLocation = OwnerLocation;

	if((StepDistanceRemaining -= Distance) <= 0.f)
	{
		if(const UFootstepSoundSet* SoundSet = PickSoundSet())
		{
			PlaySound(SoundSet->Footstep, CalculateFootstepVolume());
		}

		StepDistanceRemaining = 0.f;
		
		if(ensureAlwaysMsgf(IsValid(IntervalSpeedCurve), TEXT("Interval speed curve is not set.")))
		{
			StepDistanceRemaining = IntervalSpeedCurve->GetFloatValue(CachedSpeed);
		}
		
		if(StepDistanceRemaining == 0.f)
		{
			StepDistanceRemaining = 20.f;
		}
	}
}

UFootstepSoundSet* UFootstepSoundComponent::PickSoundSet()
{
	const FString SurfaceString = StaticEnum<EPhysicalSurface>()->GetNameStringByIndex(CachedPhysicalSurface);
	if(!ensureAlwaysMsgf(SoundSets.Contains(CachedPhysicalSurface), TEXT("Sound sets do not contain an entry for %s."), *SurfaceString))
	{
		return nullptr;
	}

	UFootstepSoundSet* PickedSet = SoundSets[CachedPhysicalSurface].Get();
	if(!ensureAlwaysMsgf(IsValid(PickedSet), TEXT("Sound set entry for %s non-valid."), *SurfaceString))
	{
		return nullptr;
	}
	
	return PickedSet;
}

float UFootstepSoundComponent::CalculateFootstepVolume() const
{
	float Volume = CachedMovementType == EFootstepMovementType::Walking ? 1.f : CrouchSoundMultiplier;

	if(ensureAlwaysMsgf(IsValid(VolumeSpeedCurve), TEXT("Volume speed curve is not valid.")))
	{
		Volume *= VolumeSpeedCurve->GetFloatValue(CachedSpeed);
	}

	return Volume;
}

float UFootstepSoundComponent::CalculateLandingVolume() const
{
	if(ensureAlwaysMsgf(IsValid(VolumeSpeedCurve), TEXT("Landing volume speed curve is not valid.")))
	{
		return VolumeSpeedCurve->GetFloatValue(CachedSpeed);
	}

	return 1.f;
}

void UFootstepSoundComponent::OnJump_Native()
{
	OnJump.Broadcast();

	if(const UFootstepSoundSet* SoundSet = PickSoundSet())
	{
		PlaySound(SoundSet->Jump);
	}
}

void UFootstepSoundComponent::OnLand_Native()
{
	UpdateSurface();
	
	OnLand.Broadcast();

	if(const UFootstepSoundSet* SoundSet = PickSoundSet())
	{
		PlaySound(SoundSet->Land, CalculateLandingVolume());
	}

	LastStepLocation = CachedOwner->GetActorLocation();
}

void UFootstepSoundComponent::OnStartMoving_Native()
{
	OnStartMoving.Broadcast();

	if(const UFootstepSoundSet* SoundSet = PickSoundSet())
	{
		PlaySound(SoundSet->Footstep, CalculateFootstepVolume());
	}
}

void UFootstepSoundComponent::OnStopMoving_Native()
{
	OnStopMoving.Broadcast();

	if(const UFootstepSoundSet* SoundSet = PickSoundSet())
	{
		PlaySound(SoundSet->Footstep, CalculateFootstepVolume());
	}
}


// Called every frame
void UFootstepSoundComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateSpeed();

	if((SurfaceAge += DeltaTime) > 0.05f && CachedSpeed > UE_SMALL_NUMBER)
	{
		SurfaceAge = 0.f;
		UpdateSurface();
	}

	ReadMovementComponent();

	PlayFootsteps();

	TimeSinceLastFootstep += DeltaTime;
}

