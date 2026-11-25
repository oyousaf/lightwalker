#pragma once

#include "CoreMinimal.h"
#include "Core/LightWalkerGameInstance.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LWScoreboard.generated.h"

/** One line of a leaderboard. */
USTRUCT()
struct FLWScore
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	int32 Score = 0;

	/** How far the run got: "WAVE 12", "THE PATH", "WON". */
	UPROPERTY()
	FString Reached;

	UPROPERTY()
	FDateTime Date = FDateTime(0);
};

/** The leaderboards on disk (Saved/SaveGames/Leaderboard.sav). */
UCLASS()
class LIGHTWALKER_API ULWScoreSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FLWScore> Story;

	UPROPERTY()
	TArray<FLWScore> Endless;

	/** Offered again next time. */
	UPROPERTY()
	FString LastName;

	/** The story has been won, which opens the endless arena. */
	UPROPERTY()
	bool bEndlessUnlocked = false;
};

/** The leaderboards, kept on this computer: the ten best runs of the story and of the endless arena. */
UCLASS()
class LIGHTWALKER_API ULWScoreboard : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 Size = 10;
	static constexpr int32 NameLength = 10;

	static ULWScoreboard* Get(const UObject* WorldContext);

	const TArray<FLWScore>& GetBoard(ERunMode Mode);
	int32 GetBest(ERunMode Mode);

	/** Whether a score makes the board. Runs started with test flags never do. */
	bool Qualifies(ERunMode Mode, int32 Score);

	/** Adds a score, saves, and returns its place (0 is first), or -1 if it didn't make it. */
	int32 Add(ERunMode Mode, const FString& Name, int32 Score, const FString& Reached);

	FString GetLastName();

	/** The endless arena opens for good once the boss is beaten in the story. */
	bool IsEndlessUnlocked();
	/** True if that's news. Runs started with test flags unlock nothing. */
	bool UnlockEndless();

	/** Letters and digits only, in capitals, no longer than NameLength. */
	static FString CleanName(const FString& Name);

private:
	ULWScoreSave& GetSave();
	void WriteSave();
	TArray<FLWScore>& GetList(ERunMode Mode);
	static const TCHAR* GetSlot();

	UPROPERTY(Transient)
	TObjectPtr<ULWScoreSave> Save;
};
