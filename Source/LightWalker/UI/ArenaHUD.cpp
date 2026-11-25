#include "UI/ArenaHUD.h"

#include "Arena/ArenaGameMode.h"
#include "CanvasItem.h"
#include "Core/LWFeel.h"
#include "Core/LightWalkerGameInstance.h"
#include "Enemies/OneEyedLiar.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Pickups/Pickup.h"
#include "Player/LightWalkerPawn.h"
#include "Player/LightWalkerPlayerController.h"
#include "Rendering/SlateRenderer.h"
#include "UI/LWStyle.h"

namespace
{
	using LWStyle::WithAlpha;
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

void AArenaHUD::DrawBox(float X, float Y, float Width, float Height, const FLinearColor& Color)
{
	FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(Width, Height), Color);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
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

void AArenaHUD::DrawCover(UTexture2D* Texture, float Alpha)
{
	if (!Texture || !Texture->GetResource())
	{
		return;
	}
	const FVector2D Screen(Canvas->ClipX, Canvas->ClipY);
	const FIntPoint Art = Texture->GetImportedSize();
	const double Scale = FMath::Max(Screen.X / FMath::Max(1, Art.X), Screen.Y / FMath::Max(1, Art.Y));
	const FVector2D Shown = Screen / (FVector2D(Art) * Scale);
	const FVector2D Start = (FVector2D::UnitVector - Shown) * 0.5f;
	FCanvasTileItem Tile(FVector2D::ZeroVector, Texture->GetResource(), Screen, Start, Start + Shown, FLinearColor(1.f, 1.f, 1.f, Alpha));
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}

void AArenaHUD::DrawHUD()
{
	Super::DrawHUD();

	const ULightWalkerGameInstance* Run = GetGameInstance<ULightWalkerGameInstance>();
	if (!Canvas || !Run || !IsShown())
	{
		return;
	}

	// The run's last picture, fading in over the game (and staying behind the leaderboard).
	const FBanner Banner = GetBanner();
	BackdropAlpha = FMath::FInterpConstantTo(BackdropAlpha, Banner.Backdrop ? 1.f : 0.f, RenderDelta, Banner.Backdrop ? 0.8f : 4.f);
	if (Banner.Backdrop)
	{
		DrawCover(LWStyle::Art(Banner.Backdrop), BackdropAlpha);
	}

	// The name entry and leaderboard have the rest of the screen to themselves.
	const ALightWalkerPlayerController* Controller = Cast<ALightWalkerPlayerController>(GetOwningPlayerController());
	if (Controller && Controller->IsRunEndShown())
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

	// Sizes are designed at 1080p and scale with the screen (by width too, so narrow screens don't overlap).
	const float Scale = FMath::Min(Canvas->ClipY / 1080.f, Canvas->ClipX / 1920.f);
	auto Px = [Scale](float Value) { return Value * Scale; };
	auto Size = [Scale](int32 Points) { return FMath::Max(1, FMath::RoundToInt(Points * Scale)); };
	const float Margin = Px(44.f);
	// The corners sit in a centred 16:9 area, so on ultrawide screens the score and lives stay near the action
	// instead of out at the far edges.
	const float SafeWidth = FMath::Min(Canvas->ClipX, Canvas->ClipY * 16.f / 9.f);
	const float SafeLeft = (Canvas->ClipX - SafeWidth) * 0.5f + Margin;
	const float SafeRight = (Canvas->ClipX + SafeWidth) * 0.5f - Margin;
	const float CentreX = Canvas->ClipX * 0.5f;
	const FLinearColor PinkGlow = WithAlpha(LWStyle::Pink, 0.7f);
	FLayout Layout;
	Layout.Scale = Scale;
	Layout.Margin = Margin;
	Layout.CentreX = CentreX;

	// Score and best, top left.
	DrawLine(TEXT("SCORE"), LWStyle::Body(Size(18), true, 300), LWStyle::Muted, SafeLeft, Margin, EAlign::Left);
	DrawLine(FString::Printf(TEXT("%06d"), Run->GetScore()), LWStyle::Body(Size(44), true, 120), LWStyle::Text, SafeLeft, Margin + Px(22.f), EAlign::Left, PinkGlow);
	DrawLine(FString::Printf(TEXT("BEST  %06d"), Run->GetBestScore()), LWStyle::Body(Size(18), true, 150), LWStyle::Cyan, SafeLeft, Margin + Px(74.f), EAlign::Left);

	DrawTopCentre(Layout);

	// Lives, top right: one hooded figure each, or a number when there's no icon art or too many to show.
	const int32 Lives = Run->GetLives();
	UTexture2D* LifeIcon = LWStyle::Art(TEXT("IconLife"));
	DrawLine(TEXT("LIVES"), LWStyle::Body(Size(18), true, 300), LWStyle::Muted, SafeRight, Margin, EAlign::Right);
	if (LifeIcon && Lives <= 6)
	{
		const float IconSize = Px(50.f);
		for (int32 Index = 0; Index < Lives; ++Index)
		{
			DrawIcon(LifeIcon, SafeRight - IconSize * (Index + 1) + Px(6.f), Margin + Px(28.f), IconSize);
		}
	}
	else
	{
		DrawLine(FString::Printf(TEXT("x%d"), Lives), LWStyle::Body(Size(44), true, 80), LWStyle::Text, SafeRight, Margin + Px(22.f), EAlign::Right, PinkGlow);
	}

	if (const ALightWalkerPawn* LightWalker = Cast<ALightWalkerPawn>(GetOwningPawn()))
	{
		// Active power-ups, bottom left: icon, name and seconds left.
		const float IconSize = Px(58.f);
		float Y = Canvas->ClipY - Margin - IconSize;
		auto DrawPower = [&](EPickupType Type, const FString& Status)
		{
			const FLinearColor Color = LWPickups::GetColor(Type);
			float TextX = SafeLeft;
			if (UTexture2D* Icon = LWPickups::GetIcon(Type))
			{
				DrawIcon(Icon, SafeLeft, Y, IconSize);
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
			const float MessageY = Canvas->ClipY * GetMessageHeight();
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

	const float BannerY = Canvas->ClipY * Banner.Height;
	if (!Banner.Title.IsEmpty())
	{
		DrawLine(Banner.Title, LWStyle::Display(Size(72), 200), Banner.Color, CentreX, BannerY, EAlign::Centre, WithAlpha(Banner.Glow, 0.75f));
	}
	if (!Banner.Detail.IsEmpty())
	{
		DrawLine(Banner.Detail, LWStyle::Body(Size(22), true, 200), LWStyle::Muted, CentreX, BannerY + Px(110.f), EAlign::Centre);
	}
	if (!Banner.Note.IsEmpty())
	{
		DrawLine(Banner.Note, LWStyle::Display(Size(34), 160), LWStyle::Gold, CentreX, BannerY + Px(160.f), EAlign::Centre, WithAlpha(LWStyle::Orange, 0.7f));
	}
}

bool AArenaHUD::IsShown() const
{
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	return GameMode && GameMode->GetPhase() != EArenaPhase::Menu;
}

void AArenaHUD::DrawTopCentre(const FLayout& Layout)
{
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (const AOneEyedLiar* Boss = GameMode->GetBoss())
	{
		// The boss's health, marked where its stages change.
		const float Width = Layout.Px(640.f);
		const float Height = Layout.Px(18.f);
		const float Left = Layout.CentreX - Width * 0.5f;
		const float Top = Layout.Margin + Layout.Px(34.f);
		const bool bGuarded = Boss->IsGuarded();
		const FLinearColor Colour = bGuarded ? LWStyle::Muted : (Boss->GetStage() == 3 ? LWStyle::Danger : LWStyle::Gold);
		DrawLine(bGuarded ? TEXT("THE ONE-EYED LIAR     BREAK THE GUARDIANS") : TEXT("THE ONE-EYED LIAR"), LWStyle::Body(Layout.Size(20), true, 300),
			bGuarded ? LWStyle::Orange : LWStyle::Text, Layout.CentreX, Layout.Margin - Layout.Px(2.f), EAlign::Centre, WithAlpha(LWStyle::Danger, 0.6f));
		DrawBox(Left - Layout.Px(3.f), Top - Layout.Px(3.f), Width + Layout.Px(6.f), Height + Layout.Px(6.f), WithAlpha(Colour, 0.25f));
		DrawBox(Left, Top, Width, Height, FLinearColor(0.02f, 0.f, 0.02f, 0.85f));
		DrawBox(Left, Top, Width * Boss->GetHealth(), Height, Colour);
		for (const float Mark : { AOneEyedLiar::StageThreeAt, AOneEyedLiar::StageTwoAt })
		{
			DrawBox(Left + Width * Mark - Layout.Px(1.f), Top, Layout.Px(2.f), Height, FLinearColor(0.f, 0.f, 0.f, 0.8f));
		}
		return;
	}
	if (GameMode->GetPhase() == EArenaPhase::Victory)
	{
		return;
	}
	DrawLine(TEXT("WAVE"), LWStyle::Body(Layout.Size(18), true, 300), LWStyle::Muted, Layout.CentreX, Layout.Margin, EAlign::Centre);
	const FString Wave = GameMode->IsEndless() ? FString::FromInt(GameMode->GetWaveNumber()) : FString::Printf(TEXT("%d / %d"), GameMode->GetWaveNumber(), GameMode->GetWaveCount());
	DrawLine(Wave, LWStyle::Body(Layout.Size(44), true, 80), LWStyle::Text,
		Layout.CentreX, Layout.Margin + Layout.Px(22.f), EAlign::Centre, WithAlpha(LWStyle::Cyan, 0.6f));
}

AArenaHUD::FBanner AArenaHUD::GetBanner() const
{
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	const ULightWalkerGameInstance* Run = GetGameInstance<ULightWalkerGameInstance>();
	FBanner Banner;
	Banner.Color = LWStyle::Pink;
	Banner.Glow = LWStyle::Cyan;
	switch (GameMode->GetPhase())
	{
	case EArenaPhase::WaveIntro:
		Banner.Title = FString::Printf(TEXT("WAVE %d"), GameMode->GetWaveNumber());
		if (GameMode->GetWaveNumber() == 1)
		{
			Banner.Detail = TEXT("WASD OR ARROWS TO MOVE     SPACE OR CLICK TO FIRE     GAMEPAD: TWIN STICKS, RT TO FIRE");
		}
		break;
	case EArenaPhase::PlayerDown:
		Banner.Title = TEXT("THE LIGHT FALTERS");
		Banner.Color = LWStyle::Danger;
		Banner.Glow = LWStyle::Magenta;
		break;
	case EArenaPhase::WaveCleared:
		Banner.Title = TEXT("WAVE CLEARED");
		if (GameMode->LeadsToPath())
		{
			Banner.Detail = TEXT("THE PATH OPENS");
		}
		else if (GameMode->LeadsToShield())
		{
			Banner.Detail = TEXT("RAISE THE SHIELD");
		}
		Banner.Color = LWStyle::Cyan;
		Banner.Glow = LWStyle::Pink;
		break;
	case EArenaPhase::GameOver:
		Banner.Title = TEXT("GAME OVER");
		Banner.Backdrop = TEXT("GameOver");
		Banner.Color = LWStyle::Danger;
		Banner.Glow = LWStyle::Magenta;
		break;
	case EArenaPhase::BossIntro:
		Banner.Title = TEXT("THE ONE-EYED LIAR");
		Banner.Color = LWStyle::Danger;
		Banner.Glow = LWStyle::Orange;
		Banner.Height = 0.56f;
		break;
	case EArenaPhase::BossFalling:
		Banner.Title = TEXT("BOSS CLEARED");
		Banner.Detail = FString::Printf(TEXT("+%d"), GameMode->GetBossBonus());
		Banner.Color = LWStyle::Gold;
		Banner.Glow = LWStyle::Pink;
		Banner.Height = 0.62f;
		break;
	case EArenaPhase::Victory:
		Banner.Title = TEXT("THE LIGHT PREVAILS");
		Banner.Backdrop = TEXT("Victory");
		Banner.Color = LWStyle::Gold;
		Banner.Glow = LWStyle::Pink;
		Banner.Detail = FString::Printf(TEXT("FINAL SCORE %d"), Run->GetScore());
		if (GameMode->HasUnlockedEndless())
		{
			Banner.Note = TEXT("ENDLESS UNLOCKED");
		}
		break;
	default:
		break;
	}
	return Banner;
}
