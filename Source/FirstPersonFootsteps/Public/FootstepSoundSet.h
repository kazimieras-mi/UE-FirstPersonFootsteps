// Copyright (c) 2023 Hitbox Initiative. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FootstepSoundSet.generated.h"

/**
 * A data asset containing a set of sounds, to be associated with a particular surface.
 */
UCLASS(Blueprintable, BlueprintType)
class FIRSTPERSONFOOTSTEPS_API UFootstepSoundSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * A simple footstep sound used for walking and running.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Footstep;

	/**
	 * Jump sound.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Jump;

	/**
	 * Land sound.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Land;
};
