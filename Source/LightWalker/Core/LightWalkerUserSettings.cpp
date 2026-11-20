#include "Core/LightWalkerUserSettings.h"

#include "Engine/Engine.h"

namespace
{
	constexpr int32 CurrentSettingsVersion = 1;
	constexpr float DefaultFrameRateLimit = 60.f;
}

ULightWalkerUserSettings* ULightWalkerUserSettings::Get()
{
	return GEngine ? Cast<ULightWalkerUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void ULightWalkerUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	FrameRateLimit = DefaultFrameRateLimit;
	bUseVSync = true;
	LightWalkerSettingsVersion = CurrentSettingsVersion;
}

void ULightWalkerUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);

	// Settings saved before the 60 fps default existed were uncapped; bring them in line once.
	if (LightWalkerSettingsVersion < 1)
	{
		FrameRateLimit = DefaultFrameRateLimit;
	}
	LightWalkerSettingsVersion = CurrentSettingsVersion;
}
