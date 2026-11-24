#include "UI/LWStyle.h"

#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GeneralProjectSettings.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"

namespace
{
	/**
	 * A runtime UFont wrapping a TTF from Content/Slate/Fonts, made once and kept. A font object (rather than a bare
	 * file path) works everywhere: UMG menus and the canvas HUD alike.
	 */
	UFont* RuntimeFont(const TCHAR* File)
	{
		static TMap<FString, TObjectPtr<UFont>> Fonts;
		if (TObjectPtr<UFont>* Found = Fonts.Find(File))
		{
			return *Found;
		}
		const FString Path = FPaths::ProjectContentDir() / TEXT("Slate/Fonts") / File;
		UFont* Font = nullptr;
		if (FPaths::FileExists(Path))
		{
			Font = NewObject<UFont>(GetTransientPackage(), NAME_None, RF_Transient);
			Font->FontCacheType = EFontCacheType::Runtime;
			Font->LegacyFontSize = 24;
			Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
			Font->AddToRoot();
		}
		Fonts.Add(File, Font);
		return Font;
	}

	FSlateFontInfo FromFile(const TCHAR* File, int32 Size, int32 LetterSpacing, const TCHAR* FallbackTypeface)
	{
		UFont* Font = RuntimeFont(File);
		FSlateFontInfo Info = Font ? FSlateFontInfo(Font, Size) : FCoreStyle::GetDefaultFontStyle(FallbackTypeface, Size);
		Info.LetterSpacing = LetterSpacing;
		return Info;
	}
}

UTexture2D* LWStyle::Art(const TCHAR* Name)
{
	static TMap<FString, TWeakObjectPtr<UTexture2D>> Cache;
	TWeakObjectPtr<UTexture2D>& Slot = Cache.FindOrAdd(Name);
	if (!Slot.IsValid())
	{
		const FString Package = FString::Printf(TEXT("/Game/UI/T_%s"), Name);
		if (FPackageName::DoesPackageExist(Package))
		{
			Slot = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("%s.T_%s"), *Package, Name));
#if WITH_EDITOR
			// The editor builds textures in the background. Slate measures one when it's first drawn, so a 9-slice drawn
			// before that finished would lose its edges.
			if (UTexture2D* Texture = Slot.Get())
			{
				Texture->BlockOnAnyAsyncBuild();
			}
#endif
		}
	}
	return Slot.Get();
}

FString LWStyle::Copyright()
{
	const UGeneralProjectSettings* Project = GetDefault<UGeneralProjectSettings>();
	return FString::Printf(TEXT("COPYRIGHT © 2026 %s. ALL RIGHTS RESERVED."), *Project->CompanyName.ToUpper());
}

FString LWStyle::Version()
{
	return TEXT("V") + GetDefault<UGeneralProjectSettings>()->ProjectVersion;
}

FSlateBrush LWStyle::ArtBrush(UTexture2D* Texture, const FVector2D& Size, const FMargin& Margin)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = Size;
	Brush.DrawAs = (Margin.Left > 0.f || Margin.Top > 0.f) ? ESlateBrushDrawType::Box : ESlateBrushDrawType::Image;
	Brush.Margin = Margin;
	return Brush;
}

FSlateFontInfo LWStyle::Display(int32 Size, int32 LetterSpacing)
{
	return FromFile(TEXT("Audiowide-Regular.ttf"), Size, LetterSpacing, TEXT("Bold"));
}

FSlateFontInfo LWStyle::Body(int32 Size, bool bBold, int32 LetterSpacing)
{
	return FromFile(bBold ? TEXT("Rajdhani-Bold.ttf") : TEXT("Rajdhani-SemiBold.ttf"), Size, LetterSpacing, bBold ? TEXT("Bold") : TEXT("Regular"));
}
