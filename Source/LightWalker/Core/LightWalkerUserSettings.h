#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "LightWalkerUserSettings.generated.h"

/**
 * The player's saved settings (GameUserSettings.ini): the engine's graphics and display settings, plus
 * volumes, vibration and the chosen hero. Set as GameUserSettingsClassName in DefaultEngine.ini.
 */
UCLASS(config = GameUserSettings, configdonotcheckdefaults)
class LIGHTWALKER_API ULightWalkerUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static ULightWalkerUserSettings* Get();

	/** New players start capped at 60 fps, with V-sync on. */
	virtual void SetToDefaults() override;
	virtual void LoadSettings(bool bForceReload = false) override;

	/** Frame-rate caps the settings menu cycles through; 0 is unlimited. */
	static TArray<float> GetFrameRateOptions() { return { 30.f, 60.f, 120.f, 144.f, 0.f }; }

	/** 0 to 1. Used by the audio pass (milestone 2); saved now so the menu is complete. */
	UPROPERTY(Config)
	float MasterVolume = 1.f;

	UPROPERTY(Config)
	float MusicVolume = 0.8f;

	UPROPERTY(Config)
	float EffectsVolume = 1.f;

	/** Controller rumble on hits, kills and pickups. */
	UPROPERTY(Config)
	bool bVibration = true;

	/** Camera shake on hits and kills (off for players who find it uncomfortable). */
	UPROPERTY(Config)
	bool bScreenShake = true;

	/** 0 = Lightwalker (male), 1 = Lightwalker (female). */
	UPROPERTY(Config)
	int32 HeroIndex = 0;

	/** Bumped when a new default needs applying to settings saved by an older build. */
	UPROPERTY(Config)
	int32 LightWalkerSettingsVersion = 0;
};
