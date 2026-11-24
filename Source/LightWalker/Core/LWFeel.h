#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "LWFeel.generated.h"

/** Moments that should be felt, not just seen. */
enum class ELWFeel : uint8
{
	Fire,
	Kill,
	DajjalHit,
	DajjalKill,
	LifeLost,
	ShieldBlock,
	Pickup,
	Book,
	WaveCleared,
	Count
};

/**
 * Game feel: camera shake (trauma that decays, applied by each mode's camera), controller rumble, hit-stop (a split second of
 * slow motion on the big moments) and full-screen colour flashes (drawn by AArenaHUD). Each moment is one call;
 * the strengths live in a table in LWFeel.cpp. Shake and rumble respect the Screen Shake and Vibration settings.
 */
UCLASS()
class LIGHTWALKER_API ULWFeelSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	static void Play(const UObject* WorldContext, ELWFeel Moment);

	void PlayMoment(ELWFeel Moment);

	/** The current screen flash colour; its alpha is 0 when there's none. */
	FLinearColor GetFlash() const;

	/** Ends any hit-stop at once (e.g. when pausing or leaving a run). */
	void CancelHitStop();

	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ULWFeelSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual ETickableTickType GetTickableTickType() const override { return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always; }

private:
	/** Real time (not game time, which hit-stop slows) when the current hit-stop and flash end. */
	double HitStopEnd = 0.0;
	double FlashStart = 0.0;
	float FlashDuration = 0.f;
	FLinearColor FlashColor = FLinearColor::Transparent;
};
