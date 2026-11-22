#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"

class UTexture2D;

/**
 * The game's look for menus and HUD: sunset neon (hot pink, magenta, orange) against cyan on deep purple, with the
 * hero's teal and gold as accents. Fonts are OFL-licensed TTFs in Content/Slate/Fonts (staged with packaged
 * builds via DirectoriesToAlwaysStageAsUFS): Audiowide for titles and buttons, Rajdhani for everything else.
 */
namespace LWStyle
{
	inline const FLinearColor Pink(1.f, 0.12f, 0.5f);
	inline const FLinearColor Magenta(0.75f, 0.12f, 1.f);
	inline const FLinearColor Orange(1.f, 0.42f, 0.1f);
	inline const FLinearColor Cyan(0.25f, 0.92f, 1.f);
	inline const FLinearColor Gold(1.f, 0.8f, 0.35f);
	inline const FLinearColor Deep(0.03f, 0.f, 0.08f);
	inline const FLinearColor Text(0.94f, 0.95f, 1.f);
	inline const FLinearColor Muted(0.62f, 0.6f, 0.85f);
	inline const FLinearColor Danger(1.f, 0.25f, 0.35f);

	/** Display face for titles, banners and buttons. */
	LIGHTWALKER_API FSlateFontInfo Display(int32 Size, int32 LetterSpacing = 0);

	/** Text face for labels, numbers and hints. */
	LIGHTWALKER_API FSlateFontInfo Body(int32 Size, bool bBold = false, int32 LetterSpacing = 0);

	/**
	 * UI artwork imported by Tools/import_ui.py as /Game/UI/T_<Name> (ButtonNormal, ButtonHover, ButtonPressed,
	 * Panel, Logo, MenuBackground, Icon*). Null if it hasn't been imported, so callers fall back to the built-in look.
	 */
	LIGHTWALKER_API UTexture2D* Art(const TCHAR* Name);

	/** A brush for a texture drawn at Size; a non-zero Margin makes it a 9-slice box that keeps its corners. */
	LIGHTWALKER_API FSlateBrush ArtBrush(UTexture2D* Texture, const FVector2D& Size, const FMargin& Margin = FMargin(0.f));
}
