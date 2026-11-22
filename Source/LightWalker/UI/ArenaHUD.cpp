#include "UI/ArenaHUD.h"

#include "Arena/ArenaGameMode.h"
#include "CanvasItem.h"
#include "Core/LWFeel.h"
#include "Core/LightWalkerGameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Pickups/Pickup.h"
#include "Player/LightWalkerPawn.h"
#include "Rendering/SlateRenderer.h"
#include "UI/LWStyle.h"

namespace
{
	using LWStyle::WithAlpha;

	const TCHAR* IconName(EPickupType Type)
	{
		switch (Type)
		{
		case EPickupType::LightOrb:     return TEXT("IconLightOrb");
		case EPickupType::SpreadShot:   return TEXT("IconSpreadShot");
		case EPickupType::PiercingBeam: return TEXT("IconPiercingBeam");
		case EPickupType::Shield:       return TEXT("IconShield");
		case EPickupType::Book:         return TEXT("IconBook");
		default:                        return TEXT("");
		}
	}
}

float AArenaHUD::MeasureWidth(const FString& Text, const FSlateFontInfo& Font) const
{
	if (!FSlateApplication::IsInitialized())
	{
		return 0.f;
	}
	return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font).X;
}

float AArenaHUD::DrawLine(const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Color, float X, float Y, EAlign Align, const FLinearColor& Shadow)
{
	const float Width = MeasureWidth(Text, Font);
	if (Align == EAlign::Centre)
	{
		X -= Width * 0.5f;
	}
	else if (Align == EAlign::Right)
	{
		X -= Width;
	}

	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
	Item.EnableShadow(WithAlpha(Shadow, Shadow.A * Color.A), FVector2D(2.f, 2.f));
	Canvas->DrawItem(Item);
	return Width;
}

void AArenaHUD::DrawIcon(UTexture2D* Texture, float X, float Y, float Size, float Alpha)
{
	if (!Texture || !Texture->GetResource())
	{
		return;
	}
	FCanvasTileItem Tile(FVector2D(X, Y), Texture->GetResource(), FVector2D(Size, Size), FLinearColor(1.f, 1.f, 1.f, Alpha));
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
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

	// A brief colour flash over the whole screen on the big moments (life lost, book, shield).
	if (const ULWFeelSubsystem* Feel = GetGameInstance()->GetSubsystem<ULWFeelSubsystem>())
	{
		const FLinearColor Flash = Feel->GetFlash();
		if (Flash.A > 0.001f)
		{
			FCanvasTileItem Tile(FVector2D::ZeroVector, FVector2D(Canvas->ClipX, Canvas->ClipY), Flash);
			Tile.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Tile);
		}
	}

	// Sizes are designed at 1080p and scale with the screen.
	const float Scale = Canvas->ClipY / 1080.f;
	auto Px = [Scale](float Value) { return Value * Scale; };
	auto Size = [Scale](int32 Points) { return FMath::Max(1, FMath::RoundToInt(Points * Scale)); };
	const float Margin = Px(44.f);
	const float CentreX = Canvas->ClipX * 0.5f;
	const FLinearColor PinkGlow = WithAlpha(LWStyle::Pink, 0.7f);
	const FLinearColor CyanGlow = WithAlpha(LWStyle::Cyan, 0.6f);

	// Score and best, top left.
	DrawLine(TEXT("SCORE"), LWStyle::Body(Size(18), true, 300), LWStyle::Muted, Margin, Margin, EAlign::Left);
	DrawLine(FString::Printf(TEXT("%06d"), Run->GetScore()), LWStyle::Body(Size(44), true, 120), LWStyle::Text, Margin, Margin + Px(22.f), EAlign::Left, PinkGlow);
	DrawLine(FString::Printf(TEXT("BEST  %06d"), Run->GetBestScore()), LWStyle::Body(Size(18), true, 150), LWStyle::Cyan, Margin, Margin + Px(74.f), EAlign::Left);

	// Wave, top centre.
	DrawLine(TEXT("WAVE"), LWStyle::Body(Size(18), true, 300), LWStyle::Muted, CentreX, Margin, EAlign::Centre);
	DrawLine(FString::Printf(TEXT("%d / %d"), GameMode->GetWaveNumber(), GameMode->GetWaveCount()), LWStyle::Body(Size(44), true, 80), LWStyle::Text, CentreX, Margin + Px(22.f), EAlign::Centre, CyanGlow);

	// Lives, top right: one hooded figure each, or a number when there's no icon art or too many to show.
	const int32 Lives = Run->GetLives();
	UTexture2D* LifeIcon = LWStyle::Art(TEXT("IconLife"));
	DrawLine(TEXT("LIVES"), LWStyle::Body(Size(18), true, 300), LWStyle::Muted, Canvas->ClipX - Margin, Margin, EAlign::Right);
	if (LifeIcon && Lives <= 6)
	{
		const float IconSize = Px(50.f);
		for (int32 Index = 0; Index < Lives; ++Index)
		{
			DrawIcon(LifeIcon, Canvas->ClipX - Margin - IconSize * (Index + 1) + Px(6.f), Margin + Px(28.f), IconSize);
		}
	}
	else
	{
		DrawLine(FString::Printf(TEXT("x%d"), Lives), LWStyle::Body(Size(44), true, 80), LWStyle::Text, Canvas->ClipX - Margin, Margin + Px(22.f), EAlign::Right, PinkGlow);
	}

	if (const ALightWalkerPawn* LightWalker = Cast<ALightWalkerPawn>(GetOwningPawn()))
	{
		// Active power-ups, bottom left: icon, name and seconds left.
		const float IconSize = Px(58.f);
		float Y = Canvas->ClipY - Margin - IconSize;
		auto DrawPower = [&](EPickupType Type, const FString& Status)
		{
			const FLinearColor Color = LWPickups::GetColor(Type);
			float TextX = Margin;
			if (UTexture2D* Icon = LWStyle::Art(IconName(Type)))
			{
				DrawIcon(Icon, Margin, Y, IconSize);
				TextX += IconSize + Px(10.f);
			}
			DrawLine(LWPickups::GetName(Type), LWStyle::Body(Size(20), true, 200), Color, TextX, Y + Px(6.f), EAlign::Left);
			DrawLine(Status, LWStyle::Body(Size(24), true, 60), LWStyle::Text, TextX, Y + Px(28.f), EAlign::Left, WithAlpha(Color, 0.7f));
			Y -= IconSize + Px(8.f);
		};
		if (LightWalker->HasShield())
		{
			DrawPower(EPickupType::Shield, TEXT("READY"));
		}
		for (const EPickupType Type : { EPickupType::PiercingBeam, EPickupType::SpreadShot, EPickupType::LightOrb })
		{
			const float Left = LightWalker->GetPowerUpTimeLeft(Type);
			if (Left > 0.f)
			{
				DrawPower(Type, FString::Printf(TEXT("%ds"), FMath::CeilToInt(Left)));
			}
		}

		// The latest pickup, below the wave counter, fading out. Book wisdom stays up longer.
		const bool bBook = LightWalker->GetPickupTitle().StartsWith(LWPickups::GetName(EPickupType::Book));
		const float ShowTime = bBook ? 6.f : 2.5f;
		const float Age = LightWalker->GetPickupMessageAge();
		if (Age < ShowTime && !LightWalker->GetPickupTitle().IsEmpty())
		{
			const float Alpha = FMath::Clamp((ShowTime - Age) / 0.6f, 0.f, 1.f);
			const float MessageY = Canvas->ClipY * 0.17f;
			const FLinearColor TitleColor = WithAlpha(bBook ? LWStyle::Gold : LWStyle::Cyan, Alpha);
			DrawLine(LightWalker->GetPickupTitle(), LWStyle::Display(Size(30), 80), TitleColor, CentreX, MessageY, EAlign::Centre, WithAlpha(bBook ? LWStyle::Orange : LWStyle::Magenta, 0.6f));
			DrawLine(LightWalker->GetPickupDetail(), LWStyle::Body(Size(bBook ? 28 : 22), true, 40), WithAlpha(LWStyle::Text, Alpha), CentreX, MessageY + Px(44.f), EAlign::Centre);
		}
	}

	// The pause menu covers the screen while paused.
	if (GetWorld()->IsPaused())
	{
		return;
	}

	FString Banner;
	FString Detail;
	FLinearColor BannerColor = LWStyle::Pink;
	FLinearColor BannerGlow = LWStyle::Cyan;
	switch (GameMode->GetPhase())
	{
	case EArenaPhase::WaveIntro:
		Banner = FString::Printf(TEXT("WAVE %d"), GameMode->GetWaveNumber());
		if (GameMode->GetWaveNumber() == 1)
		{
			Detail = TEXT("WASD OR ARROWS TO MOVE     SPACE OR CLICK TO FIRE     GAMEPAD: TWIN STICKS, RT TO FIRE");
		}
		break;
	case EArenaPhase::PlayerDown:
		Banner = TEXT("PACIFIED");
		BannerColor = LWStyle::Danger;
		BannerGlow = LWStyle::Magenta;
		break;
	case EArenaPhase::WaveCleared:
		Banner = TEXT("WAVE CLEARED");
		BannerColor = LWStyle::Cyan;
		BannerGlow = LWStyle::Pink;
		break;
	case EArenaPhase::GameOver:
		Banner = TEXT("GAME OVER");
		BannerColor = LWStyle::Danger;
		BannerGlow = LWStyle::Magenta;
		Detail = TEXT("PRESS ENTER, A OR START TO PLAY AGAIN");
		break;
	case EArenaPhase::Victory:
		Banner = TEXT("THE LIGHT PREVAILS");
		BannerColor = LWStyle::Gold;
		BannerGlow = LWStyle::Pink;
		Detail = FString::Printf(TEXT("FINAL SCORE %d     PRESS ENTER, A OR START TO PLAY AGAIN"), Run->GetScore());
		break;
	default:
		break;
	}

	const float BannerY = Canvas->ClipY * 0.38f;
	if (!Banner.IsEmpty())
	{
		DrawLine(Banner, LWStyle::Display(Size(72), 200), BannerColor, CentreX, BannerY, EAlign::Centre, WithAlpha(BannerGlow, 0.75f));
	}
	if (!Detail.IsEmpty())
	{
		DrawLine(Detail, LWStyle::Body(Size(22), true, 200), LWStyle::Muted, CentreX, BannerY + Px(110.f), EAlign::Centre);
	}
}
