#include "Core/LightWalkerGameInstance.h"

namespace
{
	// Original lines of general wisdom about light and knowledge, written for the game. Not quotations,
	// and not drawn from any scripture.
	const TCHAR* const Wisdom[] =
	{
		TEXT("A small light still pushes back the whole dark."),
		TEXT("Knowledge shared is a lamp that lights two roads."),
		TEXT("Look closely. The dark hides less than it claims."),
		TEXT("Every question is a spark. Keep striking."),
		TEXT("The patient eye sees the path the hurried foot misses."),
		TEXT("Light does not argue with the dark. It simply arrives."),
		TEXT("What you learn, no shadow can take from you."),
		TEXT("A mind kept open lets the morning in."),
		TEXT("Even the faintest star has guided someone home."),
		TEXT("Dawn never hurries, yet it never fails."),
		TEXT("To understand a fear is to carry a torch into it."),
		TEXT("The more you know, the further your light reaches."),
	};

	// Caps so a lucky run doesn't break the game.
	constexpr float MaxFireRate = 1.8f;
	constexpr float MaxLightRadius = 2.f;
	constexpr float MaxBoltSpeed = 1.8f;
	constexpr float MaxMoveSpeed = 1.3f;
	constexpr int32 MaxLives = 9;
}

void ULightWalkerGameInstance::StartRun(int32 StartingLives)
{
	Score = 0;
	Lives = FMath::Max(1, StartingLives);
	Upgrades = FRunUpgrades();
	UnreadWisdom.Reset();
	bCarriedShield = false;
}

void ULightWalkerGameInstance::AddScore(int32 Points)
{
	Score += Points;
	BestScore = FMath::Max(BestScore, Score);
}

int32 ULightWalkerGameInstance::LoseLife()
{
	Lives = FMath::Max(0, Lives - 1);
	return Lives;
}

FBookReading ULightWalkerGameInstance::ReadBook()
{
	FBookReading Reading;

	// Only offer upgrades that still have room to grow.
	TArray<int32, TInlineAllocator<5>> Options;
	if (Upgrades.FireRate < MaxFireRate)       { Options.Add(0); }
	if (Upgrades.LightRadius < MaxLightRadius) { Options.Add(1); }
	if (Upgrades.BoltSpeed < MaxBoltSpeed)     { Options.Add(2); }
	if (Upgrades.MoveSpeed < MaxMoveSpeed)     { Options.Add(3); }
	if (Lives < MaxLives)                      { Options.Add(4); }

	switch (Options.Num() > 0 ? Options[FMath::RandRange(0, Options.Num() - 1)] : -1)
	{
	case 0:
		Upgrades.FireRate = FMath::Min(Upgrades.FireRate * 1.15f, MaxFireRate);
		Reading.Upgrade = TEXT("Quicker hands: faster firing");
		break;
	case 1:
		Upgrades.LightRadius = FMath::Min(Upgrades.LightRadius * 1.25f, MaxLightRadius);
		Reading.Upgrade = TEXT("Wider light");
		break;
	case 2:
		Upgrades.BoltSpeed = FMath::Min(Upgrades.BoltSpeed * 1.2f, MaxBoltSpeed);
		Reading.Upgrade = TEXT("Swifter bolts");
		break;
	case 3:
		Upgrades.MoveSpeed = FMath::Min(Upgrades.MoveSpeed * 1.08f, MaxMoveSpeed);
		Reading.Upgrade = TEXT("Lighter steps: faster movement");
		break;
	case 4:
		++Lives;
		Reading.Upgrade = TEXT("Extra life");
		break;
	default:
		Score += 1000;
		BestScore = FMath::Max(BestScore, Score);
		Reading.Upgrade = TEXT("+1000");
		break;
	}

	if (UnreadWisdom.Num() == 0)
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Wisdom); ++Index)
		{
			UnreadWisdom.Add(Index);
		}
	}
	const int32 Pick = FMath::RandRange(0, UnreadWisdom.Num() - 1);
	Reading.Wisdom = Wisdom[UnreadWisdom[Pick]];
	UnreadWisdom.RemoveAtSwap(Pick);
	return Reading;
}
