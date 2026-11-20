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

	/** 0 to 1. Used by the audio pass (milestone 2); saved now so the menu is complete. */
	UPROPERTY(Config)
	float MasterVolume = 1.f;

	UPROPERTY(Config)
	float MusicVolume = 0.8f;

	UPROPERTY(Config)
	float EffectsVolume = 1.f;

	/** Controller rumble (used once rumble is added). */
	UPROPERTY(Config)
	bool bVibration = true;

	/** 0 = Lightwalker (male), 1 = Lightwalker (female). */
	UPROPERTY(Config)
	int32 HeroIndex = 0;
};
