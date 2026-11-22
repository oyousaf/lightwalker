#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "InputCoreTypes.h"
#include "LightWalkerUserSettings.generated.h"

/** The controls players can rebind in Settings > Controls. Each has one keyboard key and one gamepad button. */
UENUM()
enum class ELWBinding : uint8
{
	MoveUp,
	MoveLeft,
	MoveDown,
	MoveRight,
	Fire,
	Pause,
	Count UMETA(Hidden)
};

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

	/** The key (or gamepad button, with bGamepad) bound to a control, falling back to its default. */
	FKey GetBinding(ELWBinding Binding, bool bGamepad) const;
	static FKey GetDefaultBinding(ELWBinding Binding, bool bGamepad);

	/**
	 * Binds Key to a control, on the keyboard or gamepad side depending on the key. A control that already had
	 * the key takes this one's old key instead, so no key ever does two things. False if the key can't be bound.
	 */
	bool SetBinding(ELWBinding Binding, const FKey& Key);
	void ResetBindings();

	/** Mouse buttons, sticks, Escape and the arrow keys stay fixed (they always aim, cancel or move). */
	static bool CanBind(const FKey& Key);

	/** Short on-screen names: "SPACE BAR", "RT", "D-PAD UP". */
	static FString GetKeyLabel(const FKey& Key);
	static FString GetBindingLabel(ELWBinding Binding);

	/** Indexed by ELWBinding. Missing or invalid entries use the defaults. */
	UPROPERTY(Config)
	TArray<FKey> KeyboardBindings;

	UPROPERTY(Config)
	TArray<FKey> GamepadBindings;

	/** Bumped when a new default needs applying to settings saved by an older build. */
	UPROPERTY(Config)
	int32 LightWalkerSettingsVersion = 0;
};
