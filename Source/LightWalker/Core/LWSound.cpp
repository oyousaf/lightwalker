#include "Core/LWSound.h"

#include "Core/LightWalkerUserSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	struct FSoundInfo
	{
		const TCHAR* Name;
		float Volume;
		/** Random pitch spread either way (0.05 = up to 5%). */
		float PitchJitter;
		/** Seconds before this sound can play again. */
		float MinGap;
		/** Menu sounds, which must play while the game is paused. */
		bool bUI;
	};

	const FSoundInfo Sounds[] = {
		{ TEXT("Shot"),          0.30f, 0.04f, 0.f,   false },
		{ TEXT("ShotSpread"),    0.34f, 0.04f, 0.f,   false },
		{ TEXT("ShotPiercing"),  0.36f, 0.03f, 0.f,   false },
		{ TEXT("ShaytaanDeath"), 0.50f, 0.10f, 0.035f, false },
		{ TEXT("DajjalHit"),     0.45f, 0.05f, 0.03f, false },
		{ TEXT("DajjalDeath"),   0.70f, 0.05f, 0.05f, false },
		{ TEXT("LifeLost"),      0.80f, 0.f,   0.f,   false },
		{ TEXT("Respawn"),       0.55f, 0.f,   0.f,   false },
		{ TEXT("ShieldBlock"),   0.75f, 0.02f, 0.f,   false },
		{ TEXT("Glide"),         0.50f, 0.03f, 0.f,   false },
		{ TEXT("PowerUp"),       0.60f, 0.f,   0.f,   false },
		{ TEXT("PowerDown"),     0.50f, 0.f,   0.f,   false },
		{ TEXT("Book"),          0.75f, 0.f,   0.f,   false },
		{ TEXT("WaveStart"),     0.45f, 0.f,   0.f,   false },
		{ TEXT("Jump"),          0.40f, 0.05f, 0.f,   false },
		{ TEXT("DoubleJump"),    0.45f, 0.04f, 0.f,   false },
		{ TEXT("Land"),          0.40f, 0.08f, 0.05f, false },
		{ TEXT("Orb"),           0.45f, 0.04f, 0.03f, false },
		{ TEXT("Checkpoint"),    0.60f, 0.f,   0.f,   false },
		{ TEXT("LightLow"),      0.60f, 0.f,   0.f,   false },
		{ TEXT("Gate"),          0.75f, 0.f,   0.f,   false },
		{ TEXT("Deflect"),       0.55f, 0.04f, 0.03f, false },
		{ TEXT("WallHit"),       0.40f, 0.06f, 0.03f, false },
		{ TEXT("WallBreak"),     0.45f, 0.08f, 0.03f, false },
		{ TEXT("DarkBolt"),      0.45f, 0.05f, 0.05f, false },
		{ TEXT("BeamLost"),      0.75f, 0.f,   0.f,   false },
		{ TEXT("EyeBreak"),      0.85f, 0.f,   0.f,   false },
		{ TEXT("UIMove"),        0.35f, 0.02f, 0.03f, true },
		{ TEXT("UISelect"),      0.50f, 0.f,   0.f,   true },
		{ TEXT("UIPlay"),        0.60f, 0.f,   0.f,   true },
	};
	static_assert(UE_ARRAY_COUNT(Sounds) == static_cast<int32>(ELWSound::Count), "One entry per ELWSound");
}

void ULWSoundSubsystem::Play(const UObject* WorldContext, ELWSound Sound)
{
	if (const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext))
	{
		if (ULWSoundSubsystem* Subsystem = GameInstance->GetSubsystem<ULWSoundSubsystem>())
		{
			Subsystem->PlaySound(Sound);
		}
	}
}

void ULWSoundSubsystem::PlaySound(ELWSound Sound)
{
	if (!bLoaded)
	{
		bLoaded = true;
		for (const FSoundInfo& Info : Sounds)
		{
			const FString Path = FString::Printf(TEXT("/Game/Audio/SFX/SFX_%s.SFX_%s"), Info.Name, Info.Name);
			Loaded.Add(LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet));
		}
	}

	const int32 Index = static_cast<int32>(Sound);
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || !Loaded.IsValidIndex(Index) || !Loaded[Index])
	{
		return;
	}

	const FSoundInfo& Info = Sounds[Index];
	const double Now = FPlatformTime::Seconds();
	if (Now - LastPlayed[Index] < Info.MinGap)
	{
		return;
	}
	LastPlayed[Index] = Now;

	const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
	const float UserVolume = Settings ? Settings->MasterVolume * Settings->EffectsVolume : 1.f;
	if (UserVolume <= 0.f)
	{
		return;
	}
	const float Pitch = 1.f + FMath::FRandRange(-Info.PitchJitter, Info.PitchJitter);
	UGameplayStatics::PlaySound2D(World, Loaded[Index], Info.Volume * UserVolume, Pitch, 0.f, nullptr, nullptr, Info.bUI);
}
