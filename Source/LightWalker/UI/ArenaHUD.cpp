#include "UI/ArenaHUD.h"

#include "Arena/ArenaGameMode.h"
#include "Core/LightWalkerGameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"

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

	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.6f), X + 2.f * Scale, Y + 2.f * Scale, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
}

void AArenaHUD::DrawHUD()
{
	Super::DrawHUD();

	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	const ULightWalkerGameInstance* Run = GetGameInstance<ULightWalkerGameInstance>();
	if (!Canvas || !GameMode || !Run)
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

	FString Banner;
	FString Detail;
	FLinearColor BannerColor = Primary;

	switch (GameMode->GetPhase())
	{
	case EArenaPhase::WaveIntro:
		Banner = FString::Printf(TEXT("WAVE %d"), GameMode->GetWaveNumber());
		if (GameMode->GetWaveNumber() == 1)
		{
			Detail = TEXT("Arrows or left stick to move    WASD or right stick to fire");
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
		Detail = TEXT("Press Enter or Start to play again");
		break;
	case EArenaPhase::Victory:
		Banner = TEXT("THE LIGHT PREVAILS");
		Detail = FString::Printf(TEXT("Final score %d    Press Enter or Start to play again"), Run->GetScore());
		break;
	default:
		break;
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
