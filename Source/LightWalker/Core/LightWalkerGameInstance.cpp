#include "Core/LightWalkerGameInstance.h"

void ULightWalkerGameInstance::StartRun(int32 StartingLives)
{
	Score = 0;
	Lives = FMath::Max(1, StartingLives);
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
