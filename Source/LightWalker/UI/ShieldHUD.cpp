#include "UI/ShieldHUD.h"

#include "Engine/World.h"
#include "Player/LightWalkerPlayerController.h"
#include "Shield/ShieldGameMode.h"
#include "UI/LWStyle.h"

bool AShieldHUD::IsShown() const
{
	return GetWorld()->GetAuthGameMode<AShieldGameMode>() != nullptr;
}

void AShieldHUD::DrawTopCentre(const FLayout& Layout)
{
	const AShieldGameMode* GameMode = GetWorld()->GetAuthGameMode<AShieldGameMode>();
	const FString Label = GameMode->IsGreatEye() ? GameMode->GetWallName() : FString::Printf(TEXT("WALL  %d / %d"), GameMode->GetWallNumber(), GameMode->GetWallCount());
	DrawLine(Label, LWStyle::Body(Layout.Size(18), true, 300), LWStyle::Muted, Layout.CentreX, Layout.Margin, EAlign::Centre);

	const float Width = Layout.Px(360.f);
	const float Height = Layout.Px(12.f);
	const float Left = Layout.CentreX - Width * 0.5f;
	const float Top = Layout.Margin + Layout.Px(34.f);
	const FLinearColor Colour = GameMode->IsGreatEye() ? LWStyle::Orange : LWStyle::Magenta;
	DrawBox(Left - Layout.Px(3.f), Top - Layout.Px(3.f), Width + Layout.Px(6.f), Height + Layout.Px(6.f), LWStyle::WithAlpha(Colour, 0.25f));
	DrawBox(Left, Top, Width, Height, FLinearColor(0.01f, 0.f, 0.03f, 0.85f));
	DrawBox(Left, Top, Width * GameMode->GetWallLeft(), Height, Colour);
}

AArenaHUD::FBanner AShieldHUD::GetBanner() const
{
	const AShieldGameMode* GameMode = GetWorld()->GetAuthGameMode<AShieldGameMode>();
	FBanner Banner;
	Banner.Color = LWStyle::Cyan;
	Banner.Glow = LWStyle::Magenta;
	Banner.Height = 0.56f;  // in the clear between the wall and Lightwalker
	switch (GameMode->GetPhase())
	{
	case EShieldPhase::WallIntro:
		Banner.Title = GameMode->GetWallName();
		if (GameMode->GetWallNumber() == 1)
		{
			Banner.Detail = TEXT("TURN YOUR OWN LIGHT AGAINST THE DARK");
		}
		else if (GameMode->IsGreatEye())
		{
			Banner.Detail = TEXT("BREAK THE EYE OF HORUS AND THE WALL FALLS");
		}
		break;
	case EShieldPhase::Playing:
		if (GameMode->IsBeamHeld())
		{
			const ALightWalkerPlayerController* Controller = Cast<ALightWalkerPlayerController>(GetOwningPlayerController());
			Banner.Detail = Controller ? Controller->GetShieldHint() : FString();
		}
		break;
	case EShieldPhase::Down:
		Banner.Title = GameMode->WasBeamLost() ? TEXT("THE BEAM IS LOST") : TEXT("PACIFIED");
		Banner.Color = LWStyle::Danger;
		break;
	case EShieldPhase::WallCleared:
		Banner.Title = TEXT("WALL CLEARED");
		break;
	case EShieldPhase::Finished:
		Banner.Title = TEXT("ALL WALLS CLEARED");
		Banner.Color = LWStyle::Gold;
		Banner.Detail = TEXT("BACK TO THE ARENA");
		break;
	case EShieldPhase::GameOver:
		Banner.Title = TEXT("GAME OVER");
		Banner.Backdrop = TEXT("GameOver");
		Banner.Color = LWStyle::Danger;
		break;
	default:
		break;
	}
	return Banner;
}
