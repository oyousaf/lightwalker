#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LightWalkerGameInstance.generated.h"

/** Lasting upgrades from books of knowledge. They last for the rest of the run, across waves and modes. */
USTRUCT(BlueprintType)
struct FRunUpgrades
{
	GENERATED_BODY()

	/** Multiplies shots per second. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float FireRate = 1.f;

	/** Multiplies the radius of Lightwalker's light. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float LightRadius = 1.f;

	/** Multiplies bolt speed. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float BoltSpeed = 1.f;

	/** Multiplies Lightwalker's movement speed. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float MoveSpeed = 1.f;
};

/** What's being played: the story (every mode, to the boss) or the arena without end. */
UENUM()
enum class ERunMode : uint8
{
	Story,
	Endless
};

/** How the arena map should begin when it loads. */
enum class EArenaStart : uint8
{
	/** The main menu (after the intro, the first time). */
	Menu,
	/** A new run from wave 1. */
	NewRun,
	/** The run in progress, at GetNextWave. */
	Continue
};

/** What a book of knowledge gave and said. */
struct FBookReading
{
	FString Upgrade;
	FString Wisdom;
};

/**
 * State that outlives a single map: the current run's score, lives and upgrades, and the best score this session.
 * Every mode (Arena, The Path, The Shield) reads and writes the same run, so a run can chain through them.
 */
UCLASS()
class LIGHTWALKER_API ULightWalkerGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	void StartRun(int32 StartingLives);
	void AddScore(int32 Points);

	/** Removes a life and returns how many are left. */
	int32 LoseLife();

	/** Reads a book of knowledge: grants one random lasting upgrade and returns it with a line of wisdom. */
	FBookReading ReadBook();

	int32 GetScore() const { return Score; }
	int32 GetLives() const { return Lives; }
	/** The best of this kind of run, this session or on the leaderboard. */
	int32 GetBestScore() const { return BestScore[static_cast<int32>(RunMode)]; }

	ERunMode GetRunMode() const { return RunMode; }
	void SetRunMode(ERunMode Mode) { RunMode = Mode; }
	const FRunUpgrades& GetUpgrades() const { return Upgrades; }

	/** Set before travelling to the arena; read once by its game mode. */
	void SetArenaStart(EArenaStart Start, int32 WaveIndex = 0) { ArenaStart = Start; NextWave = WaveIndex; }
	EArenaStart TakeArenaStart() { const EArenaStart Start = ArenaStart; ArenaStart = EArenaStart::Menu; return Start; }
	int32 GetNextWave() const { return NextWave; }

	/** The intro plays once per launch, not every time the arena map loads. */
	bool bIntroSeen = false;

	/** An unused shield comes along between modes. */
	bool bCarriedShield = false;

private:
	EArenaStart ArenaStart = EArenaStart::Menu;
	int32 NextWave = 0;

	int32 Score = 0;
	int32 Lives = 0;
	int32 BestScore[2] = { 0, 0 };
	ERunMode RunMode = ERunMode::Story;
	FRunUpgrades Upgrades;

	/** Wisdom lines not yet shown this run, so they don't repeat until all have been read. */
	TArray<int32> UnreadWisdom;
};
