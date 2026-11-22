#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/HUD.h"
#include "ArenaHUD.generated.h"

class UTexture2D;

/**
 * The in-game HUD, drawn on the canvas in the game's neon style (UI/LWStyle): score and best top left, the wave
 * top centre, lives as hooded-figure icons top right, active power-ups as icons with timers bottom left, pickup
 * messages and the centre banners. Uses the ChatGPT icon art when imported, text otherwise.
 */
UCLASS()
class LIGHTWALKER_API AArenaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	enum class EAlign : uint8 { Left, Centre, Right };

	/** Draws text with a coloured glow-shadow; returns its width. */
	float DrawLine(const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Color, float X, float Y, EAlign Align,
		const FLinearColor& Shadow = FLinearColor(0.f, 0.f, 0.f, 0.6f));

	float MeasureWidth(const FString& Text, const FSlateFontInfo& Font) const;
	void DrawIcon(UTexture2D* Texture, float X, float Y, float Size, float Alpha = 1.f);
};
