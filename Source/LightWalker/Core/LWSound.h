#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LWSound.generated.h"

class USoundBase;

/** Every sound effect, made by Tools/sfx/make_sfx.py and imported to /Game/Audio/SFX/SFX_<Name>. */
enum class ELWSound : uint8
{
	Shot, ShotSpread, ShotPiercing,
	ShaytaanDeath, DajjalHit, DajjalDeath,
	LifeLost, Respawn, ShieldBlock, Glide,
	PowerUp, PowerDown, Book, WaveStart,
	UIMove, UISelect, UIPlay,
	Count
};

/**
 * Plays the sound effects through the Master and Effects volume settings. Each sound has its own level, a little
 * random pitch so repeats don't sound mechanical, and a minimum gap so a swarm dying at once doesn't pile up.
 * Missing assets are skipped silently.
 */
UCLASS()
class LIGHTWALKER_API ULWSoundSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Plays a sound from anywhere with a world. */
	static void Play(const UObject* WorldContext, ELWSound Sound);

	void PlaySound(ELWSound Sound);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> Loaded;

	double LastPlayed[static_cast<int32>(ELWSound::Count)] = {};
	bool bLoaded = false;
};
