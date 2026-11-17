#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaWaveSet.generated.h"

/** Chasers switch to Speed once this many enemies (of any kind) or fewer are left in the wave. */
USTRUCT(BlueprintType)
struct FChaserSpeedStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0))
	int32 RemainingAtMost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0, Units = "cm/s"))
	float Speed = 0.f;
};

/** One wave of the arena. */
USTRUCT(BlueprintType)
struct FArenaWave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0))
	int32 ShaytaanCount = 15;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0))
	int32 DajjalCount = 0;

	/** Chaser speed at the start of the wave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0, Units = "cm/s"))
	float ShaytaanSpeed = 136.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FChaserSpeedStep> SpeedSteps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0, Units = "cm/s"))
	float DajjalSpeed = 100.f;

	int32 GetEnemyCount() const { return ShaytaanCount + DajjalCount; }
};

/**
 * A list of waves, so they can be tuned in the editor without recompiling
 * (Content Browser: Add > Miscellaneous > Data Asset > ArenaWaveSet, then assign it to the game mode).
 */
UCLASS(BlueprintType)
class LIGHTWALKER_API UArenaWaveSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
	TArray<FArenaWave> Waves;
};
