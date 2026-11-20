#include "Core/LightWalkerUserSettings.h"

#include "Engine/Engine.h"

ULightWalkerUserSettings* ULightWalkerUserSettings::Get()
{
	return GEngine ? Cast<ULightWalkerUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}
