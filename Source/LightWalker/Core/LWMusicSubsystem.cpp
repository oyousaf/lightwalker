#include "Core/LWMusicSubsystem.h"

#include "Components/AudioComponent.h"
#include "Core/LightWalkerUserSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	const TCHAR* const MenuIntroPath = TEXT("/Game/Audio/Music/Menu/MenuTheme_Intro.MenuTheme_Intro");
	const TCHAR* const MenuLoopPath = TEXT("/Game/Audio/Music/Menu/MenuTheme_Loop.MenuTheme_Loop");
	const TCHAR* const StemPaths[] = {
		TEXT("/Game/Audio/Music/Combat/Combat_1_Strings.Combat_1_Strings"),
		TEXT("/Game/Audio/Music/Combat/Combat_2_Melody.Combat_2_Melody"),
		TEXT("/Game/Audio/Music/Combat/Combat_3_808.Combat_3_808"),
		TEXT("/Game/Audio/Music/Combat/Combat_4_Drums.Combat_4_Drums"),
		TEXT("/Game/Audio/Music/Combat/Combat_5_Counter.Combat_5_Counter"),
		TEXT("/Game/Audio/Music/Combat/Combat_6_HatRolls.Combat_6_HatRolls"),
	};
	const TCHAR* const StingPaths[] = {
		TEXT("/Game/Audio/Music/Combat/Sting_WaveCleared.Sting_WaveCleared"),
		TEXT("/Game/Audio/Music/Combat/Sting_GameOver.Sting_GameOver"),
	};

	/** The wave each stem joins in. */
	constexpr int32 StemFromWave[] = { 1, 1, 4, 8, 12, 12 };

	/** How far the combat music drops while ducked. */
	constexpr float DuckLevel = 0.25f;
}

ULWMusicSubsystem* ULWMusicSubsystem::Get(const UObject* WorldContext)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext);
	return GameInstance ? GameInstance->GetSubsystem<ULWMusicSubsystem>() : nullptr;
}

float ULWMusicSubsystem::GetUserVolume() const
{
	const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
	return Settings ? Settings->MasterVolume * Settings->MusicVolume : 1.f;
}

UAudioComponent* ULWMusicSubsystem::StartTrack(const TCHAR* AssetPath, float StartVolume)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	USoundBase* Sound = World ? LoadObject<USoundBase>(nullptr, AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
	if (!Sound)
	{
		return nullptr;
	}

	UAudioComponent* Component = UGameplayStatics::CreateSound2D(World, Sound, 1.f, 1.f, 0.f, nullptr, true, false);
	if (Component)
	{
		// Music carries on under the pause menu.
		Component->bIsUISound = true;
		Component->SetVolumeMultiplier(FMath::Max(StartVolume * GetUserVolume(), 0.001f));
		Component->Play();
		Playing.RemoveAll([](const UAudioComponent* Other) { return !Other || !Other->IsPlaying(); });
		Playing.Add(Component);
	}
	return Component;
}

void ULWMusicSubsystem::FadeTo(FTrack& Track, float Target, float Seconds)
{
	Track.Target = Target;
	Track.FadeRate = Seconds > 0.f ? FMath::Max(FMath::Abs(Target - Track.Volume) / Seconds, 0.01f) : 1000.f;
}

void ULWMusicSubsystem::PlayMenu()
{
	for (FTrack& Stem : CombatStems)
	{
		FadeTo(Stem, 0.f, 1.5f);
	}

	const bool bMenuPlaying = (MenuIntro.Component.IsValid() && MenuIntro.Component->IsPlaying()) ||
		(MenuLoop.Component.IsValid() && MenuLoop.Component->IsPlaying());
	if (bMenuPlaying)
	{
		FadeTo(MenuIntro, 1.f, 1.f);
		FadeTo(MenuLoop, 1.f, 1.f);
		return;
	}

	MenuIntro = FTrack();
	MenuLoop = FTrack();
	MenuIntro.Component = StartTrack(MenuIntroPath, 1.f);
	MenuIntro.Volume = MenuIntro.Target = 1.f;
	bMenuWaitingForLoop = MenuIntro.Component.IsValid();
	if (!bMenuWaitingForLoop)
	{
		MenuLoop.Component = StartTrack(MenuLoopPath, 1.f);
		MenuLoop.Volume = MenuLoop.Target = 1.f;
	}
}

void ULWMusicSubsystem::PlayCombat()
{
	FadeTo(MenuIntro, 0.f, 1.5f);
	FadeTo(MenuLoop, 0.f, 1.5f);
	bMenuWaitingForLoop = false;

	// Every stem starts at the same moment so they stay locked together; the silent ones are faded in later.
	for (FTrack& Stem : CombatStems)
	{
		if (Stem.Component.IsValid())
		{
			Stem.Component->Stop();
		}
	}
	CombatStems.Reset();
	for (const TCHAR* Path : StemPaths)
	{
		FTrack& Stem = CombatStems.AddDefaulted_GetRef();
		Stem.Component = StartTrack(Path, 0.f);
	}
	DuckRemaining = 0.f;
	SetCombatWave(1);
}

void ULWMusicSubsystem::EnsureCombat()
{
	const bool bPlaying = CombatStems.ContainsByPredicate([](const FTrack& Stem)
	{
		return Stem.Component.IsValid() && Stem.Component->IsPlaying();
	});
	if (!bPlaying)
	{
		PlayCombat();
	}
}

void ULWMusicSubsystem::SetCombatWave(int32 WaveNumber)
{
	CombatWave = WaveNumber;
	for (int32 Index = 0; Index < CombatStems.Num(); ++Index)
	{
		FadeTo(CombatStems[Index], WaveNumber >= StemFromWave[Index] ? 1.f : 0.f, 2.5f);
	}
}

void ULWMusicSubsystem::DuckCombat(float Seconds)
{
	DuckRemaining = FMath::Max(DuckRemaining, Seconds);
}

void ULWMusicSubsystem::PlaySting(ELWSting Sting)
{
	StartTrack(StingPaths[static_cast<int32>(Sting)], 1.f);
	if (Sting == ELWSting::GameOver)
	{
		for (FTrack& Stem : CombatStems)
		{
			FadeTo(Stem, 0.f, 0.6f);
		}
	}
	else
	{
		DuckCombat(2.5f);
	}
}

void ULWMusicSubsystem::StopAll(float FadeSeconds)
{
	for (FTrack* Track : { &MenuIntro, &MenuLoop })
	{
		FadeTo(*Track, 0.f, FadeSeconds);
	}
	for (FTrack& Stem : CombatStems)
	{
		FadeTo(Stem, 0.f, FadeSeconds);
	}
	bMenuWaitingForLoop = false;
}

void ULWMusicSubsystem::Tick(float DeltaTime)
{
	// The intro hands over to the loop the moment it finishes.
	if (bMenuWaitingForLoop && (!MenuIntro.Component.IsValid() || !MenuIntro.Component->IsPlaying()))
	{
		bMenuWaitingForLoop = false;
		MenuLoop.Component = StartTrack(MenuLoopPath, MenuIntro.Target);
		MenuLoop.Volume = MenuLoop.Target = MenuIntro.Target;
	}

	DuckRemaining = FMath::Max(0.f, DuckRemaining - DeltaTime);
	DuckGain = FMath::FInterpConstantTo(DuckGain, DuckRemaining > 0.f ? DuckLevel : 1.f, DeltaTime, DuckRemaining > 0.f ? 4.f : 0.8f);
	const float UserVolume = GetUserVolume();

	auto Update = [&](FTrack& Track, float Extra)
	{
		Track.Volume = FMath::FInterpConstantTo(Track.Volume, Track.Target, DeltaTime, Track.FadeRate);
		if (UAudioComponent* Component = Track.Component.Get())
		{
			// Never fully zero, so a silent stem keeps its place in time instead of being culled.
			Component->SetVolumeMultiplier(FMath::Max(Track.Volume * Extra * UserVolume, 0.001f));
		}
	};

	Update(MenuIntro, 1.f);
	Update(MenuLoop, 1.f);
	for (FTrack& Stem : CombatStems)
	{
		Update(Stem, DuckGain);
	}
}
