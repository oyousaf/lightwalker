#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaWaveSet.generated.h"

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
	float ShaytaanSpeed = 100.f;

	/**
	 * Chasers speed up smoothly from ShaytaanSpeed to this as the wave thins out; the last enemy standing
	 * moves at this speed. (2014 jumped in steps: 6, 8, 10, 15 px per 220 ms tick = 136, 182, 227, 341 cm/s.)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0, Units = "cm/s"))
	float ShaytaanTopSpeed = 180.f;

	/** Shape of the ramp: 1 is even, higher keeps them slow for longer and saves the rush for the last few. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = 0.1))
	float SpeedRampCurve = 1.5f;

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
