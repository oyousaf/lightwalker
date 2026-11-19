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
	int32 GetBestScore() const { return BestScore; }
	const FRunUpgrades& GetUpgrades() const { return Upgrades; }

private:
	int32 Score = 0;
	int32 Lives = 0;
	int32 BestScore = 0;
	FRunUpgrades Upgrades;

	/** Wisdom lines not yet shown this run, so they don't repeat until all have been read. */
	TArray<int32> UnreadWisdom;
};
