#include "Core/LWScoreboard.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "LightWalker.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	/** -LWTestBoard (non-shipping) uses a board of its own, so tests never touch the player's. */
	bool IsTestBoard()
	{
#if UE_BUILD_SHIPPING
		return false;
#else
		static const bool bTest = FParse::Param(FCommandLine::Get(), TEXT("LWTestBoard"));
		return bTest;
#endif
	}

	/** A run that skipped ahead or played itself. */
	bool IsTestRun()
	{
#if UE_BUILD_SHIPPING
		return false;
#else
		static const bool bTest = FCString::Stristr(FCommandLine::Get(), TEXT("-LWStart")) || FCString::Stristr(FCommandLine::Get(), TEXT("-LWAuto"));
		return bTest;
#endif
	}
}

ULWScoreboard* ULWScoreboard::Get(const UObject* WorldContext)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext);
	return GameInstance ? GameInstance->GetSubsystem<ULWScoreboard>() : nullptr;
}

const TCHAR* ULWScoreboard::GetSlot()
{
	return IsTestBoard() ? TEXT("LeaderboardTest") : TEXT("Leaderboard");
}

ULWScoreSave& ULWScoreboard::GetSave()
{
	if (!Save)
	{
		Save = Cast<ULWScoreSave>(UGameplayStatics::LoadGameFromSlot(GetSlot(), 0));
		if (!Save)
		{
			Save = Cast<ULWScoreSave>(UGameplayStatics::CreateSaveGameObject(ULWScoreSave::StaticClass()));
		}
	}
	return *Save;
}

TArray<FLWScore>& ULWScoreboard::GetList(ERunMode Mode)
{
	return Mode == ERunMode::Endless ? GetSave().Endless : GetSave().Story;
}

const TArray<FLWScore>& ULWScoreboard::GetBoard(ERunMode Mode)
{
	return GetList(Mode);
}

int32 ULWScoreboard::GetBest(ERunMode Mode)
{
	const TArray<FLWScore>& List = GetList(Mode);
	return List.Num() > 0 ? List[0].Score : 0;
}

bool ULWScoreboard::Qualifies(ERunMode Mode, int32 Score)
{
	if (Score <= 0 || (IsTestRun() && !IsTestBoard()))
	{
		return false;
	}
	const TArray<FLWScore>& List = GetList(Mode);
	return List.Num() < Size || Score > List.Last().Score;
}

int32 ULWScoreboard::Add(ERunMode Mode, const FString& Name, int32 Score, const FString& Reached)
{
	if (!Qualifies(Mode, Score))
	{
		return -1;
	}

	FLWScore Entry;
	Entry.Name = CleanName(Name);
	if (Entry.Name.IsEmpty())
	{
		Entry.Name = TEXT("WALKER");
	}
	Entry.Score = Score;
	Entry.Reached = Reached;
	Entry.Date = FDateTime::Now();
	GetSave().LastName = Entry.Name;

	// Below anyone with the same score: they got there first.
	TArray<FLWScore>& List = GetList(Mode);
	int32 Place = 0;
	while (Place < List.Num() && List[Place].Score >= Score)
	{
		++Place;
	}
	List.Insert(Entry, Place);
	if (List.Num() > Size)
	{
		List.SetNum(Size);
	}
	WriteSave();
	return Place < Size ? Place : -1;
}

void ULWScoreboard::WriteSave()
{
	if (!UGameplayStatics::SaveGameToSlot(&GetSave(), GetSlot(), 0))
	{
		UE_LOG(LogLightWalker, Warning, TEXT("The leaderboard couldn't be saved."));
	}
}

bool ULWScoreboard::IsEndlessUnlocked()
{
	// A win on the board counts too: it may be from before this was kept.
	return GetSave().bEndlessUnlocked || GetSave().Story.ContainsByPredicate([](const FLWScore& Score) { return Score.Reached == TEXT("WON"); });
}

bool ULWScoreboard::UnlockEndless()
{
	if (IsEndlessUnlocked() || (IsTestRun() && !IsTestBoard()))
	{
		return false;
	}
	GetSave().bEndlessUnlocked = true;
	WriteSave();
	return true;
}

FString ULWScoreboard::GetLastName()
{
	return GetSave().LastName;
}

FString ULWScoreboard::CleanName(const FString& Name)
{
	FString Clean;
	for (const TCHAR Character : Name.ToUpper())
	{
		if ((Character >= TEXT('A') && Character <= TEXT('Z')) || (Character >= TEXT('0') && Character <= TEXT('9')))
		{
			Clean.AppendChar(Character);
		}
	}
	return Clean.Left(NameLength);
}
