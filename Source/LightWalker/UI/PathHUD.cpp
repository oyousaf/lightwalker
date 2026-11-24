#include "UI/PathHUD.h"

#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Path/PathGameMode.h"
#include "Player/LightWalkerPlayerController.h"
#include "UI/LWStyle.h"

bool APathHUD::IsShown() const
{
	return GetWorld()->GetAuthGameMode<APathGameMode>() != nullptr;
}

void APathHUD::DrawTopCentre(const FLayout& Layout)
{
	const APathGameMode* GameMode = GetWorld()->GetAuthGameMode<APathGameMode>();
	const float Light = GameMode->GetLight();
	const float Width = Layout.Px(420.f);
	const float Left = Layout.CentreX - Width * 0.5f;
	const float Top = Layout.Margin + Layout.Px(34.f);
	const float Height = Layout.Px(16.f);

	// The lantern: gold while there's plenty, turning to a pulsing red as it runs low.
	const bool bLow = Light < 0.25f;
	const float Pulse = bLow ? 0.65f + 0.35f * FMath::Sin(GetWorld()->GetTimeSeconds() * 9.f) : 1.f;
	const FLinearColor Colour = bLow ? LWStyle::Danger : FMath::Lerp(LWStyle::Orange, LWStyle::Gold, FMath::Clamp((Light - 0.25f) / 0.4f, 0.f, 1.f));
	DrawLine(bLow ? TEXT("THE LIGHT IS FADING") : TEXT("LIGHT"), LWStyle::Body(Layout.Size(18), true, 300), bLow ? LWStyle::WithAlpha(LWStyle::Danger, Pulse) : LWStyle::Muted,
		Layout.CentreX, Layout.Margin, EAlign::Centre);
	DrawBox(Left - Layout.Px(3.f), Top - Layout.Px(3.f), Width + Layout.Px(6.f), Height + Layout.Px(6.f), LWStyle::WithAlpha(Colour, 0.25f * Pulse));
	DrawBox(Left, Top, Width, Height, FLinearColor(0.01f, 0.f, 0.03f, 0.85f));
	DrawBox(Left, Top, Width * Light, Height, LWStyle::WithAlpha(Colour, Pulse));
	DrawBox(Left, Top, Width * Light, Height * 0.35f, FLinearColor(1.f, 1.f, 1.f, 0.25f * Pulse));

	// How far along: a thin line with a mark for each lantern and one for Lightwalker.
	const float LineY = Top + Height + Layout.Px(18.f);
	const float Line = FMath::Max(1.f, Layout.Px(3.f));
	DrawBox(Left, LineY, Width, Line, LWStyle::WithAlpha(LWStyle::Muted, 0.35f));
	DrawBox(Left, LineY, Width * GameMode->GetProgress(), Line, LWStyle::WithAlpha(LWStyle::Cyan, 0.9f));
	const TArray<float> Marks = GameMode->GetCheckpointMarks();
	for (int32 Index = 0; Index < Marks.Num(); ++Index)
	{
		const float Size = Layout.Px(8.f);
		DrawBox(Left + Width * Marks[Index] - Size * 0.5f, LineY + Line * 0.5f - Size * 0.5f, Size, Size,
			Index <= GameMode->GetCheckpointsLit() ? LWStyle::Gold : LWStyle::WithAlpha(LWStyle::Muted, 0.6f));
	}
	const float Dot = Layout.Px(12.f);
	DrawBox(Left + Width * GameMode->GetProgress() - Dot * 0.5f, LineY + Line * 0.5f - Dot * 0.5f, Dot, Dot, LWStyle::Text);
}

AArenaHUD::FBanner APathHUD::GetBanner() const
{
	const APathGameMode* GameMode = GetWorld()->GetAuthGameMode<APathGameMode>();
	FBanner Banner;
	Banner.Color = LWStyle::Gold;
	Banner.Glow = LWStyle::Pink;
	Banner.Height = 0.2f;  // above the action
	switch (GameMode->GetPhase())
	{
	case EPathPhase::Intro:
		Banner.Title = TEXT("THE PATH");
		Banner.Detail = TEXT("REACH THE GATE BEFORE YOUR LIGHT GOES OUT");
		break;
	case EPathPhase::Running:
		// The controls, for the first few seconds of the first stretch.
		if (GameMode->GetCheckpointsLit() == 0 && GameMode->GetProgress() < 0.06f)
		{
			const ALightWalkerPlayerController* Controller = Cast<ALightWalkerPlayerController>(GetOwningPlayerController());
			Banner.Detail = Controller ? Controller->GetPathHint() : FString();
		}
		break;
	case EPathPhase::Down:
		Banner.Title = GameMode->WentDark() ? TEXT("YOUR LIGHT WENT OUT") : TEXT("PACIFIED");
		Banner.Color = LWStyle::Danger;
		Banner.Glow = LWStyle::Magenta;
		break;
	case EPathPhase::Finished:
		Banner.Title = TEXT("PATH CLEARED");
		Banner.Height = 0.72f;  // clear of the gate
		Banner.Detail = FString::Printf(TEXT("LIGHT BONUS  +%d"), GameMode->GetFinishBonus());
		break;
	case EPathPhase::GameOver:
		Banner.Title = TEXT("GAME OVER");
		Banner.Backdrop = TEXT("GameOver");
		Banner.Color = LWStyle::Danger;
		Banner.Glow = LWStyle::Magenta;
		break;
	default:
		break;
	}
	return Banner;
}
