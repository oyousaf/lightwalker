#include "Core/LWFeel.h"

#include "Core/LWGameMode.h"
#include "Core/LightWalkerUserSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	struct FFeel
	{
		/** Camera trauma added (0-1; shake grows with its square, so small values stay subtle). */
		float Shake;
		/** Rumble strength (0-1) and length in seconds; bHeavy uses the big low motors as well as the small ones. */
		float Rumble;
		float RumbleTime;
		bool bHeavy;
		/** Seconds of near-freeze. */
		float HitStop;
		/** Full-screen flash: colour (alpha = peak strength) and length. */
		FLinearColor Flash;
		float FlashTime;
	};

	const FFeel Moments[] = {
		/* Fire        */ { 0.f,   0.12f, 0.05f, false, 0.f,   FLinearColor::Transparent, 0.f },
		/* Kill        */ { 0.12f, 0.3f,  0.08f, false, 0.f,   FLinearColor::Transparent, 0.f },
		/* DajjalHit   */ { 0.1f,  0.25f, 0.06f, false, 0.f,   FLinearColor::Transparent, 0.f },
		/* DajjalKill  */ { 0.38f, 0.6f,  0.18f, true,  0.05f, FLinearColor(1.f, 0.75f, 0.25f, 0.12f), 0.2f },
		/* LifeLost    */ { 0.75f, 1.f,   0.35f, true,  0.12f, FLinearColor(1.f, 0.1f, 0.45f, 0.45f), 0.6f },
		/* ShieldBlock */ { 0.4f,  0.7f,  0.2f,  true,  0.06f, FLinearColor(0.3f, 1.f, 0.45f, 0.25f), 0.35f },
		/* Pickup      */ { 0.f,   0.2f,  0.08f, false, 0.f,   FLinearColor::Transparent, 0.f },
		/* Book        */ { 0.15f, 0.45f, 0.3f,  true,  0.f,   FLinearColor(1.f, 0.8f, 0.3f, 0.35f), 0.7f },
		/* WaveCleared */ { 0.2f,  0.3f,  0.2f,  true,  0.f,   FLinearColor(0.25f, 0.9f, 1.f, 0.12f), 0.5f },
		/* BossHit     */ { 0.03f, 0.15f, 0.04f, false, 0.f,   FLinearColor::Transparent, 0.f },
		/* BossDown    */ { 0.9f,  1.f,   0.5f,  true,  0.12f, FLinearColor(1.f, 0.85f, 0.5f, 0.45f), 0.9f },
	};
	static_assert(UE_ARRAY_COUNT(Moments) == static_cast<int32>(ELWFeel::Count), "One entry per ELWFeel");

	/** Game speed during hit-stop. */
	constexpr float HitStopDilation = 0.03f;
}

void ULWFeelSubsystem::Play(const UObject* WorldContext, ELWFeel Moment)
{
	if (const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext))
	{
		if (ULWFeelSubsystem* Subsystem = GameInstance->GetSubsystem<ULWFeelSubsystem>())
		{
			Subsystem->PlayMoment(Moment);
		}
	}
}

void ULWFeelSubsystem::PlayMoment(ELWFeel Moment)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	const FFeel& Feel = Moments[static_cast<int32>(Moment)];
	const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();

	if (Feel.Shake > 0.f && (!Settings || Settings->bScreenShake))
	{
		if (ALWGameMode* GameMode = World->GetAuthGameMode<ALWGameMode>())
		{
			GameMode->AddCameraTrauma(Feel.Shake);
		}
	}

	if (Feel.Rumble > 0.f && (!Settings || Settings->bVibration))
	{
		if (APlayerController* Controller = World->GetFirstPlayerController())
		{
			Controller->PlayDynamicForceFeedback(Feel.Rumble, Feel.RumbleTime, Feel.bHeavy, true, Feel.bHeavy, true);
		}
	}

	const double Now = FPlatformTime::Seconds();
	if (Feel.HitStop > 0.f && !World->IsPaused())
	{
		HitStopEnd = FMath::Max(HitStopEnd, Now + Feel.HitStop);
		UGameplayStatics::SetGlobalTimeDilation(World, HitStopDilation);
	}

	if (Feel.FlashTime > 0.f && Feel.Flash.A >= FlashColor.A * (1.f - (Now - FlashStart) / FMath::Max(FlashDuration, 0.01f)))
	{
		FlashColor = Feel.Flash;
		FlashStart = Now;
		FlashDuration = Feel.FlashTime;
	}
}

FLinearColor ULWFeelSubsystem::GetFlash() const
{
	if (FlashDuration <= 0.f)
	{
		return FLinearColor::Transparent;
	}
	const float T = static_cast<float>((FPlatformTime::Seconds() - FlashStart) / FlashDuration);
	if (T >= 1.f)
	{
		return FLinearColor::Transparent;
	}
	FLinearColor Color = FlashColor;
	Color.A *= FMath::Square(1.f - T);  // quick in, eased out
	return Color;
}

void ULWFeelSubsystem::CancelHitStop()
{
	if (HitStopEnd > 0.0)
	{
		HitStopEnd = 0.0;
		if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
		{
			UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
		}
	}
}

void ULWFeelSubsystem::Deinitialize()
{
	CancelHitStop();
	Super::Deinitialize();
}

void ULWFeelSubsystem::Tick(float DeltaTime)
{
	// Hit-stop is timed in real seconds, since game time is what it slows down.
	if (HitStopEnd > 0.0 && FPlatformTime::Seconds() >= HitStopEnd)
	{
		CancelHitStop();
	}
}
