#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "LWMusicSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

UENUM()
enum class ELWSting : uint8
{
	WaveCleared,
	GameOver
};

/**
 * The game's music. Menu: an intro that plays once into a loop. Combat: six stems that start together and stay
 * in sync, faded in and out as layers by wave (see SetCombatWave). Stings play over the top. Everything runs
 * through the Master and Music volume settings, keeps playing in the pause menu, and is silently skipped if
 * its asset hasn't been imported (Tools/import_audio.py).
 */
UCLASS()
class LIGHTWALKER_API ULWMusicSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	static ULWMusicSubsystem* Get(const UObject* WorldContext);

	void PlayMenu();
	void PlayCombat();

	/** Starts the combat music unless it's already playing (arriving from another mode). */
	void EnsureCombat();

	/** Brings in the stem layers for this wave: 1-3 strings and melody, 4-7 + 808, 8-11 + drums, 12+ + counter and hat rolls. */
	void SetCombatWave(int32 WaveNumber);

	/** Pulls the combat music right down for a moment (a life lost), then lets it come back. */
	void DuckCombat(float Seconds);

	void PlaySting(ELWSting Sting);
	void StopAll(float FadeSeconds = 1.f);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ULWMusicSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual ETickableTickType GetTickableTickType() const override { return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always; }

private:
	struct FTrack
	{
		TWeakObjectPtr<UAudioComponent> Component;
		float Volume = 0.f;
		float Target = 0.f;
		/** Volume change per second. */
		float FadeRate = 0.5f;
	};

	UAudioComponent* StartTrack(const TCHAR* AssetPath, float StartVolume);
	void FadeTo(FTrack& Track, float Target, float Seconds);
	float GetUserVolume() const;

	/** Keeps the playing components from being garbage collected; the tracks only point at them. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> Playing;

	FTrack MenuIntro;
	FTrack MenuLoop;
	TArray<FTrack> CombatStems;
	float DuckRemaining = 0.f;
	/** Eases down quickly when ducked and back up slowly. */
	float DuckGain = 1.f;
	bool bMenuWaitingForLoop = false;
	int32 CombatWave = 1;
};
