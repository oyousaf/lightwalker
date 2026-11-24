#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/HUD.h"
#include "ArenaHUD.generated.h"

class UTexture2D;

/**
 * The in-game HUD (the arena's; APathHUD changes the parts other modes show differently), drawn on the canvas in the game's neon style (UI/LWStyle): score and best top left, the wave
 * top centre, lives as hooded-figure icons top right, active power-ups as icons with timers bottom left, pickup
 * messages and the centre banners. Uses the ChatGPT icon art when imported, text otherwise.
 */
UCLASS()
class LIGHTWALKER_API AArenaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	enum class EAlign : uint8 { Left, Centre, Right };

	/** Sizes are designed at 1080p and scale with the screen. */
	struct FLayout
	{
		float Scale = 1.f;
		float Margin = 0.f;
		float CentreX = 0.f;
		float Px(float Value) const { return Value * Scale; }
		int32 Size(int32 Points) const { return FMath::Max(1, FMath::RoundToInt(Points * Scale)); }
	};

	/** The big words in the middle of the screen. */
	struct FBanner
	{
		FString Title;
		FString Detail;
		FLinearColor Color = FLinearColor::White;
		FLinearColor Glow = FLinearColor::White;
		/** How far down the screen, 0-1. */
		float Height = 0.38f;
	};

	// What each mode shows; the rest (score, lives, power-ups, messages) is shared.
	virtual bool IsShown() const;
	virtual void DrawTopCentre(const FLayout& Layout);
	virtual FBanner GetBanner() const;

	/** Draws text with a coloured glow-shadow; returns its width. */
	float DrawLine(const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Color, float X, float Y, EAlign Align,
		const FLinearColor& Shadow = FLinearColor(0.f, 0.f, 0.f, 0.6f));

	float MeasureWidth(const FString& Text, const FSlateFontInfo& Font) const;
	void DrawIcon(UTexture2D* Texture, float X, float Y, float Size, float Alpha = 1.f);
};
