#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LightWalkerGameInstance.generated.h"

/**
 * State that outlives a single map: the current run's score and lives, and the best score this session.
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

	int32 GetScore() const { return Score; }
	int32 GetLives() const { return Lives; }
	int32 GetBestScore() const { return BestScore; }

private:
	int32 Score = 0;
	int32 Lives = 0;
	int32 BestScore = 0;
};
