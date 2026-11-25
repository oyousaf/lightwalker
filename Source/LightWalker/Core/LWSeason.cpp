#include "Core/LWSeason.h"

#include "Core/LightWalkerUserSettings.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "UI/LWStyle.h"

namespace
{
	/** Ramadan 1, Eid al-Fitr and Eid al-Adha as yyyymmdd, from the Umm al-Qura calendar. Sightings can differ by a day. */
	struct FHijriYear { int32 Ramadan, Fitr, Adha; };
	const FHijriYear HijriYears[] = {
		{ 20260218, 20260320, 20260527 },
		{ 20270208, 20270309, 20270516 },
		{ 20280128, 20280226, 20280505 },
		{ 20290116, 20290214, 20290424 },
		{ 20300105, 20300204, 20300413 },
		{ 20301226, 20310124, 20310402 },
		{ 20311215, 20320114, 20320322 },
		{ 20321204, 20330102, 20330311 },
		{ 20331123, 20331223, 20340301 },
		{ 20341112, 20341212, 20350218 },
		{ 20351101, 20351201, 20360207 },
		{ 20361020, 20361119, 20370126 },
		{ 20371010, 20371108, 20380116 },
		{ 20380930, 20381029, 20390105 },
		{ 20390919, 20391019, 20391226 },
		{ 20400907, 20401007, 20401214 },
		{ 20410828, 20410926, 20411204 },
		{ 20420817, 20420915, 20421123 },
		{ 20430806, 20430904, 20431112 },
		{ 20440726, 20440824, 20441031 },
		{ 20450715, 20450814, 20451021 },
		{ 20460705, 20460803, 20461010 },
		{ 20470624, 20470724, 20470930 },
		{ 20480612, 20480712, 20480919 },
		{ 20490602, 20490701, 20490908 },
		{ 20500522, 20500620, 20500828 },
		{ 20510511, 20510610, 20510817 },
		{ 20520430, 20520529, 20520805 },
		{ 20530420, 20530519, 20530726 },
		{ 20540409, 20540509, 20540715 },
	};

	FDateTime FromNumber(int32 Number)
	{
		return FDateTime(Number / 10000, (Number / 100) % 100, Number % 100);
	}

	const TCHAR* SuffixFor(ELWSeason Season)
	{
		switch (Season)
		{
		case ELWSeason::Winter:  return TEXT("Winter");
		case ELWSeason::Spring:  return TEXT("Spring");
		case ELWSeason::Autumn:  return TEXT("Autumn");
		case ELWSeason::Ramadan: return TEXT("Ramadan");
		case ELWSeason::Eid:     return TEXT("Eid");
		default:                 return nullptr;
		}
	}

	/** Whether Windows' region is south of the equator, where the seasons are the other way round. */
	bool IsSouthern()
	{
		static const bool bSouthern = []
		{
			FString Language, Region;
			FPlatformMisc::GetDefaultLocale().Split(TEXT("-"), &Language, &Region, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			static const TCHAR* const South[] = { TEXT("AU"), TEXT("NZ"), TEXT("ZA"), TEXT("AR"), TEXT("BR"), TEXT("CL"), TEXT("UY"), TEXT("PY"), TEXT("BO"), TEXT("PE"),
				TEXT("ZW"), TEXT("ZM"), TEXT("BW"), TEXT("NA"), TEXT("MZ"), TEXT("MG"), TEXT("MW"), TEXT("AO"), TEXT("LS"), TEXT("SZ"), TEXT("FJ"), TEXT("PG") };
			for (const TCHAR* Country : South)
			{
				if (Region == Country)
				{
					return true;
				}
			}
			return false;
		}();
		return bSouthern;
	}

	/** Today's, looked up again only when the day changes. */
	ELWSeason Today()
	{
		static FDateTime Day(0);
		static ELWSeason Season = ELWSeason::None;
		const FDateTime Now = FDateTime::Now().GetDate();
		if (Now != Day)
		{
			Day = Now;
			Season = LWSeason::ForDate(Now, IsSouthern());
		}
		return Season;
	}
}

ELWSeason LWSeason::ForDate(const FDateTime& Date, bool bSouthern)
{
	const FDateTime Day = Date.GetDate();
	auto Within = [&Day](const FDateTime& First, const FDateTime& Last) { return Day >= First && Day <= Last; };
	const FTimespan OneDay = FTimespan::FromDays(1);
	for (const FHijriYear& Year : HijriYears)
	{
		// The Eids run long enough to cover a sighting a day later; Ramadan begins a day early.
		if (Within(FromNumber(Year.Fitr), FromNumber(Year.Fitr) + OneDay * 4) || Within(FromNumber(Year.Adha), FromNumber(Year.Adha) + OneDay * 4))
		{
			return ELWSeason::Eid;
		}
		if (Within(FromNumber(Year.Ramadan) - OneDay, FromNumber(Year.Fitr) - OneDay))
		{
			return ELWSeason::Ramadan;
		}
	}

	// December to February, March to May, June to August, September to November.
	static const ELWSeason ByQuarter[] = { ELWSeason::Winter, ELWSeason::Spring, ELWSeason::None, ELWSeason::Autumn };
	return ByQuarter[((Day.GetMonth() % 12) / 3 + (bSouthern ? 2 : 0)) % 4];
}

ELWSeason LWSeason::Current()
{
#if !UE_BUILD_SHIPPING
	// Dev only: -LWSeason=N shows one (1 winter, 2 spring, 3 autumn, 4 Ramadan, 5 Eid, 0 none).
	static const int32 Forced = [] { int32 Value = -1; FParse::Value(FCommandLine::Get(), TEXT("LWSeason="), Value); return Value; }();
	if (Forced >= 0)
	{
		return static_cast<ELWSeason>(FMath::Min(Forced, static_cast<int32>(ELWSeason::Count) - 1));
	}
#endif
	const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
	const int32 Choice = Settings ? Settings->SeasonChoice : ULightWalkerUserSettings::SeasonAuto;
	return Choice >= 0 && Choice < static_cast<int32>(ELWSeason::Count) ? static_cast<ELWSeason>(Choice) : Today();
}

FString LWSeason::GetName(ELWSeason Season)
{
	const TCHAR* Suffix = SuffixFor(Season);
	return Suffix ? FString(Suffix).ToUpper() : FString(TEXT("OFF"));
}

UTexture2D* LWSeason::OwnArt(const TCHAR* Name)
{
	const ELWSeason Season = Current();
	const TCHAR* Suffix = SuffixFor(Season);
	UTexture2D* Own = Suffix ? LWStyle::Art(*FString::Printf(TEXT("%s_%s"), Name, Suffix)) : nullptr;
	// Eid shares Ramadan's lanterns and crescent.
	return Own || Season != ELWSeason::Eid ? Own : LWStyle::Art(*FString::Printf(TEXT("%s_Ramadan"), Name));
}

UTexture2D* LWSeason::Art(const TCHAR* Name)
{
	UTexture2D* Own = OwnArt(Name);
	return Own ? Own : LWStyle::Art(Name);
}

bool LWSeason::GetArenaColors(FLinearColor& TopEdge, FLinearColor& SideEdges, FLinearColor& Grid)
{
	switch (Current())
	{
	case ELWSeason::Winter:
		TopEdge = FLinearColor(0.22f, 0.6f, 1.f);
		SideEdges = FLinearColor(0.32f, 0.2f, 1.f);
		Grid = FLinearColor(0.25f, 0.5f, 1.f);
		return true;
	case ELWSeason::Spring:
		TopEdge = FLinearColor(1.f, 0.3f, 0.55f);
		SideEdges = FLinearColor(0.25f, 1.f, 0.5f);
		Grid = FLinearColor(0.55f, 0.25f, 0.9f);
		return true;
	case ELWSeason::Autumn:
		TopEdge = FLinearColor(1.f, 0.16f, 0.04f);
		SideEdges = FLinearColor(1.f, 0.55f, 0.1f);
		Grid = FLinearColor(0.7f, 0.2f, 0.45f);
		return true;
	case ELWSeason::Ramadan:
		TopEdge = FLinearColor(1.f, 0.62f, 0.18f);
		SideEdges = FLinearColor(0.08f, 0.75f, 0.7f);
		Grid = FLinearColor(0.12f, 0.45f, 0.65f);
		return true;
	case ELWSeason::Eid:
		TopEdge = FLinearColor(1.f, 0.65f, 0.2f);
		SideEdges = FLinearColor(1.f, 0.15f, 0.6f);
		Grid = FLinearColor(0.15f, 0.6f, 0.7f);
		return true;
	default:
		return false;
	}
}

TArray<FLWWeatherLayer> LWSeason::GetWeather()
{
	// Dim, so nothing in the air is mistaken for a bolt.
	using EKind = FLWWeatherLayer::EKind;
	switch (Current())
	{
	case ELWSeason::Winter:
		return { { EKind::Flake, 380, FLinearColor(0.6f, 0.78f, 1.f), 0.8f, FVector2D(8.f, 15.f), FVector2D(90.f, 190.f), 45.f, 34.f } };
	case ELWSeason::Spring:
		return { { EKind::Scrap, 170, FLinearColor(1.f, 0.42f, 0.62f), 0.7f, FVector2D(20.f, 32.f), FVector2D(60.f, 110.f), 60.f, 55.f } };
	case ELWSeason::Autumn:
		return { { EKind::Streak, 300, FLinearColor(0.5f, 0.6f, 0.85f), 0.35f, FVector2D(2.6f, 3.8f), FVector2D(1500.f, 2000.f), 160.f, 0.f },
			{ EKind::Scrap, 100, FLinearColor(1.f, 0.36f, 0.06f), 0.7f, FVector2D(22.f, 36.f), FVector2D(90.f, 160.f), 120.f, 60.f } };
	case ELWSeason::Ramadan:
		return { { EKind::Ember, 190, FLinearColor(1.f, 0.62f, 0.22f), 0.9f, FVector2D(6.f, 12.f), FVector2D(30.f, 70.f), 8.f, 24.f } };
	case ELWSeason::Eid:
		return { { EKind::Ember, 220, FLinearColor(1.f, 0.62f, 0.22f), 1.1f, FVector2D(7.f, 13.f), FVector2D(45.f, 95.f), 10.f, 26.f },
			{ EKind::Ember, 110, FLinearColor(1.f, 0.2f, 0.55f), 1.f, FVector2D(7.f, 12.f), FVector2D(45.f, 95.f), 10.f, 26.f } };
	default:
		return {};
	}
}
