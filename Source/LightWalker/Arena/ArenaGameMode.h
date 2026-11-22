#pragma once

#include "CoreMinimal.h"
#include "Arena/ArenaWaveSet.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaGameMode.generated.h"

class AArena;
class ADajjal;
class AEnemyBase;
class ALightWalkerPawn;
class APickup;
class AShaytaan;
class ULightWalkerGameInstance;

UENUM(BlueprintType)
enum class EArenaPhase : uint8
{
	/** Title and main menu: Lightwalker stands in the arena, enemies lurk at the edge of the light. */
	Menu,
	/** Enemies are placed but frozen; the wave banner is up. */
	WaveIntro,
	Fighting,
	/** Lightwalker was pacified and is waiting to respawn. */
	PlayerDown,
	WaveCleared,
	GameOver,
	Victory
};

/**
 * Arena mode: the original Lightwalker. Clear every wave of shaytaan and dajjal with three lives.
 */
UCLASS()
class LIGHTWALKER_API AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaGameMode();

	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	void HandleEnemyKilled(AEnemyBase* Enemy);
	void HandlePlayerTouched(ALightWalkerPawn* LightWalker);

	/** Confirm pressed: starts a new run after a game over or a win (ignored for a moment after it ends,
	    so fire buttons still held from the last fight don't skip the end screen). */
	void RequestRestart();

	bool IsRunOver() const { return Phase == EArenaPhase::GameOver || Phase == EArenaPhase::Victory; }

	/** Play pressed: the camera sweeps up to the game view and wave 1 begins. */
	void StartFromMenu();

	/** Back to the main menu from a run. */
	void ReturnToMenu();

	/** Starts the run again from wave 1 (pause menu). */
	void RestartRun() { StartRun(); }

	EArenaPhase GetPhase() const { return Phase; }
	int32 GetWaveNumber() const { return FMath::Min(WaveIndex + 1, GetWaveCount()); }
	int32 GetWaveCount() const { return GetWaves().Num(); }
	int32 GetEnemiesRemaining() const { return Enemies.Num(); }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Arena")
	TSubclassOf<AArena> ArenaClass;

	UPROPERTY(EditDefaultsOnly, Category = "Arena")
	TSubclassOf<AShaytaan> ShaytaanClass;

	UPROPERTY(EditDefaultsOnly, Category = "Arena")
	TSubclassOf<ADajjal> DajjalClass;

	/** Optional. When set, its waves replace DefaultWaves. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Waves")
	TObjectPtr<UArenaWaveSet> WaveSet;

	/** 15 waves: the two 2014 levels (15 chasers, then 30 chasers and 5 wanderers), then 13 that build from there. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Waves")
	TArray<FArenaWave> DefaultWaves;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Pickups")
	TSubclassOf<APickup> PickupClass;

	/** Chance a killed dajjal drops a power-up. They take three shots, so they pay out more. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Pickups", meta = (ClampMin = 0, ClampMax = 1))
	float DajjalDropChance = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Pickups", meta = (ClampMin = 0, ClampMax = 1))
	float ShaytaanDropChance = 0.03f;

	/** Chance of a book of knowledge instead (at most one per wave). */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Pickups", meta = (ClampMin = 0, ClampMax = 1))
	float DajjalBookChance = 0.06f;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Pickups", meta = (ClampMin = 0, ClampMax = 1))
	float ShaytaanBookChance = 0.005f;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules")
	int32 StartingLives = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules")
	int32 PointsPerKill = 100;

	/** Dajjal take three shots, so they're worth more. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules")
	int32 PointsPerDajjal = 300;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules", meta = (Units = "s"))
	float WaveIntroDuration = 2.f;

	/** 2014: 4 seconds between levels. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules", meta = (Units = "s"))
	float WaveClearedDuration = 4.f;

	/** 2014: 3 seconds before Lightwalker returns. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules", meta = (Units = "s"))
	float RespawnDelay = 3.f;

	/** Time after a respawn when enemies can't hurt Lightwalker. 0 matches 2014. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules", meta = (Units = "s"))
	float RespawnGrace = 1.f;

	/** How long the end screens ignore the restart button. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules", meta = (Units = "s"))
	float RestartLockout = 1.f;

	/** Enemies never spawn inside this box around the centre, where Lightwalker starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Spawning")
	FVector2D SafeZoneHalfExtent = FVector2D(500.f, 525.f);

	/** A respawn spot needs at least this much space from every enemy. 2014: 90 px. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Spawning")
	float RespawnClearance = 450.f;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Spawning")
	int32 RespawnAttempts = 35;

	/** Height of the play plane above the arena floor. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Spawning")
	float PlaneHeight = 50.f;

private:
	const TArray<FArenaWave>& GetWaves() const;
	ULightWalkerGameInstance* GetRun() const;
	ALightWalkerPawn* GetLightWalker() const;

	void StartRun();
	void EnterMenu(bool bInstantCamera);
	void BeginWaveIntro();
	void BeginFighting();
	void SpawnWave(const FArenaWave& Wave);
	void ApplyChaserSpeed();
	void FinishWave();
	void AdvanceWave();
	void RespawnLightWalker();
	void ClearField();
	void SetEnemiesActive(bool bActive);
	void TryDropPickup(const FVector& Location, bool bFromDajjal);
	bool bBookDroppedThisWave = false;
	void SetPhase(EArenaPhase NewPhase, float Duration, void (AArenaGameMode::*Next)());

	FVector RandomEnemySpawn() const;
	FVector FindRespawnPoint() const;

	UPROPERTY(Transient)
	TObjectPtr<AArena> Arena;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEnemyBase>> Enemies;

#if !UE_BUILD_SHIPPING
	/** Dev only: -LWShots=N takes N high-res screenshots three seconds apart, then quits. */
	void TakeDevScreenshot();
	int32 DevShotsRemaining = 0;
	int32 DevShotIndex = 0;
	FTimerHandle DevShotTimer;
#endif

	EArenaPhase Phase = EArenaPhase::Menu;
	float PhaseStartTime = 0.f;
	int32 WaveIndex = 0;
	FTimerHandle PhaseTimer;
};
