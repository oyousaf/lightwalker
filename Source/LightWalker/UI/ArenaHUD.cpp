#include "UI/ArenaHUD.h"

#include "Arena/ArenaGameMode.h"
#include "Core/LightWalkerGameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Pickups/Pickup.h"
#include "Player/LightWalkerPawn.h"

namespace
{
	const FLinearColor Primary(0.55f, 0.95f, 1.f);
	const FLinearColor Muted(0.55f, 0.6f, 0.8f);
	const FLinearColor Danger(1.f, 0.3f, 0.45f);
}

void AArenaHUD::DrawLine(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale, EAlign Align)
{
	UFont* Font = GEngine->GetLargeFont();
	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(Text, Width, Height, Font, Scale);

	if (Align == EAlign::Centre)
	{
		X -= Width * 0.5f;
	}
	else if (Align == EAlign::Right)
	{
		X -= Width;
	}

	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.6f * Color.A), X + 2.f * Scale, Y + 2.f * Scale, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
}

void AArenaHUD::DrawHUD()
{
	Super::DrawHUD();

	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	const ULightWalkerGameInstance* Run = GetGameInstance<ULightWalkerGameInstance>();
	if (!Canvas || !GameMode || !Run || GameMode->GetPhase() == EArenaPhase::Menu)
	{
		return;
	}

	const float Scale = Canvas->ClipY / 1080.f;
	const float Margin = 40.f * Scale;
	const float CentreX = Canvas->ClipX * 0.5f;

	DrawLine(FString::Printf(TEXT("SCORE  %06d"), Run->GetScore()), Primary, Margin, Margin, 1.6f * Scale, EAlign::Left);
	DrawLine(FString::Printf(TEXT("BEST  %06d"), Run->GetBestScore()), Muted, Margin, Margin + 42.f * Scale, 1.1f * Scale, EAlign::Left);
	DrawLine(FString::Printf(TEXT("WAVE %d/%d"), GameMode->GetWaveNumber(), GameMode->GetWaveCount()), Primary, CentreX, Margin, 1.6f * Scale, EAlign::Centre);
	DrawLine(FString::Printf(TEXT("LIVES  %d"), Run->GetLives()), Primary, Canvas->ClipX - Margin, Margin, 1.6f * Scale, EAlign::Right);

	if (const ALightWalkerPawn* LightWalker = Cast<ALightWalkerPawn>(GetOwningPawn()))
	{
		// Active power-ups, bottom left, with seconds left.
		float Y = Canvas->ClipY - Margin - 36.f * Scale;
		if (LightWalker->HasShield())
		{
			DrawLine(LWPickups::GetName(EPickupType::Shield), LWPickups::GetColor(EPickupType::Shield), Margin, Y, 1.2f * Scale, EAlign::Left);
			Y -= 38.f * Scale;
		}
		for (const EPickupType Type : { EPickupType::PiercingBeam, EPickupType::SpreadShot, EPickupType::LightOrb })
		{
			const float Left = LightWalker->GetPowerUpTimeLeft(Type);
			if (Left > 0.f)
			{
				DrawLine(FString::Printf(TEXT("%s  %d"), *LWPickups::GetName(Type), FMath::CeilToInt(Left)), LWPickups::GetColor(Type),
					Margin, Y, 1.2f * Scale, EAlign::Left);
				Y -= 38.f * Scale;
			}
		}

		// The latest pickup, under the wave counter, fading out. Book wisdom stays up longer.
		const bool bBook = LightWalker->GetPickupTitle().StartsWith(LWPickups::GetName(EPickupType::Book));
		const float ShowTime = bBook ? 6.f : 2.5f;
		const float Age = LightWalker->GetPickupMessageAge();
		if (Age < ShowTime && !LightWalker->GetPickupTitle().IsEmpty())
		{
			const float Alpha = FMath::Clamp((ShowTime - Age) / 0.6f, 0.f, 1.f);
			FLinearColor TitleColor = bBook ? LWPickups::GetColor(EPickupType::Book) : Primary;
			FLinearColor DetailColor = bBook ? FLinearColor(1.f, 0.95f, 0.8f) : Muted;
			TitleColor.A = DetailColor.A = Alpha;
			const float MessageY = Canvas->ClipY * 0.18f;
			DrawLine(LightWalker->GetPickupTitle(), TitleColor, CentreX, MessageY, 1.5f * Scale, EAlign::Centre);
			DrawLine(LightWalker->GetPickupDetail(), DetailColor, CentreX, MessageY + 46.f * Scale, (bBook ? 1.3f : 1.1f) * Scale, EAlign::Centre);
		}
	}

	FString Banner;
	FString Detail;
	FLinearColor BannerColor = Primary;

	switch (GameMode->GetPhase())
	{
	case EArenaPhase::WaveIntro:
		Banner = FString::Printf(TEXT("WAVE %d"), GameMode->GetWaveNumber());
		if (GameMode->GetWaveNumber() == 1)
		{
			Detail = TEXT("WASD or arrows to move    Space or click to fire    Gamepad: twin sticks, RT to fire");
		}
		break;
	case EArenaPhase::PlayerDown:
		Banner = TEXT("PACIFIED");
		BannerColor = Danger;
		break;
	case EArenaPhase::WaveCleared:
		Banner = TEXT("WAVE CLEARED");
		break;
	case EArenaPhase::GameOver:
		Banner = TEXT("GAME OVER");
		BannerColor = Danger;
		Detail = TEXT("Press Enter, A or Start to play again");
		break;
	case EArenaPhase::Victory:
		Banner = TEXT("THE LIGHT PREVAILS");
		Detail = FString::Printf(TEXT("Final score %d    Press Enter, A or Start to play again"), Run->GetScore());
		break;
	default:
		break;
	}

	// The pause menu covers the screen while paused.
	if (GetWorld()->IsPaused())
	{
		return;
	}

	const float BannerY = Canvas->ClipY * 0.4f;
	if (!Banner.IsEmpty())
	{
		DrawLine(Banner, BannerColor, CentreX, BannerY, 3.f * Scale, EAlign::Centre);
	}
	if (!Detail.IsEmpty())
	{
		DrawLine(Detail, Muted, CentreX, BannerY + 90.f * Scale, 1.2f * Scale, EAlign::Centre);
	}
}
