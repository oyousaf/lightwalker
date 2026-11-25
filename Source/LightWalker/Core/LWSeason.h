#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** How the game is dressed. Only decoration: nothing is locked behind a season. */
enum class ELWSeason : uint8
{
	/** The usual look, and summer's. */
	None,
	Winter,
	Spring,
	Autumn,
	Ramadan,
	Eid,
	Count
};

/** One kind of thing in the air. */
struct FLWWeatherLayer
{
	enum class EKind : uint8 { Flake, Streak, Scrap, Ember };
	EKind Kind = EKind::Flake;
	int32 Count = 0;
	FLinearColor Color = FLinearColor::White;
	float Glow = 1.f;
	/** Size of the thing in cm, smallest and largest. */
	FVector2D Size = FVector2D(5.f, 9.f);
	/** Cm a second downwards (negative rises), slowest and fastest. */
	FVector2D Fall = FVector2D(100.f, 180.f);
	/** Sideways drift, and how far it sways. */
	float Wind = 30.f;
	float Sway = 25.f;
};

namespace LWSeason
{
	/** The season being shown: the player's choice in Settings, or by the computer's date and region. */
	LIGHTWALKER_API ELWSeason Current();

	/** By date alone. Southern swaps the seasons round; Ramadan and the Eids come first wherever you are. */
	LIGHTWALKER_API ELWSeason ForDate(const FDateTime& Date, bool bSouthern);

	/** "WINTER". */
	LIGHTWALKER_API FString GetName(ELWSeason Season);

	/** T_<Name>_<Season> if it's been imported, otherwise the usual T_<Name>. */
	LIGHTWALKER_API UTexture2D* Art(const TCHAR* Name);
	/** The season's own only (null in the usual look, or without the art). */
	LIGHTWALKER_API UTexture2D* OwnArt(const TCHAR* Name);

	/** The arena's edges and grid. False in the usual look. */
	LIGHTWALKER_API bool GetArenaColors(FLinearColor& TopEdge, FLinearColor& SideEdges, FLinearColor& Grid);

	LIGHTWALKER_API TArray<FLWWeatherLayer> GetWeather();
}
