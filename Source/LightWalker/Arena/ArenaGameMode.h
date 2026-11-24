#pragma once

#include "CoreMinimal.h"
#include "Arena/ArenaWaveSet.h"
#include "Core/LWGameMode.h"
#include "ArenaGameMode.generated.h"

class AArena;
class ADajjal;
class AOneEyedLiar;
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
	/** Fading out to the next mode. */
	Travelling,
	/** The One-Eyed Liar is coming down. */
	BossIntro,
	/** The One-Eyed Liar is falling. */
	BossFalling,
	GameOver,
	Victory
};

/**
 * Arena mode: the original Lightwalker. Clear every wave of shaytaan and dajjal with three lives.
 */
UCLASS()
class LIGHTWALKER_API AArenaGameMode : public ALWGameMode
{
	GENERATED_BODY()

public:
	AArenaGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	virtual void HandleEnemyKilled(AEnemyBase* Enemy) override;
	virtual void HandlePlayerTouched(ALightWalkerPawn* LightWalker) override;

	/** Confirm pressed: starts a new run after a game over or a win (ignored for a moment after it ends,
	    so fire buttons still held from the last fight don't skip the end screen). */
	virtual void RequestRestart() override;

	virtual bool IsRunOver() const override { return Phase == EArenaPhase::GameOver || Phase == EArenaPhase::Victory; }

	/** Play pressed: the camera sweeps up to the game view and wave 1 begins. */
	virtual void StartFromMenu() override;

	/** Back to the main menu from a run. */
	virtual void ReturnToMenu() override;

	/** Starts the run again from wave 1 (pause menu). */
	virtual void RestartRun() override { StartRun(); }

	virtual bool IsInMenu() const override { return Phase == EArenaPhase::Menu; }
	virtual FString GetReached() const override;
	bool IsEndless() const;
	virtual void AddCameraTrauma(float Amount) override;

	/** The wave just cleared leads into The Path, or The Shield, rather than straight to the next wave. */
	bool LeadsToPath() const;
	bool LeadsToShield() const;

	EArenaPhase GetPhase() const { return Phase; }
	int32 GetWaveNumber() const { return IsEndless() ? WaveIndex + 1 : FMath::Min(WaveIndex + 1, GetWaveCount()); }
	int32 GetWaveCount() const { return GetWaves().Num(); }
	int32 GetEnemiesRemaining() const { return Enemies.Num(); }
	AArena* GetArena() const { return Arena; }
	const TArray<TObjectPtr<AEnemyBase>>& GetEnemies() const { return Enemies; }

	/** Null except during the boss fight. */
	AOneEyedLiar* GetBoss() const { return Boss; }
	int32 GetBossBonus() const { return BossBonus; }

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

	/** Arena 1 is waves 1 to this; then comes The Path, and the arena picks up at the next wave. 0 = never. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules")
	int32 PathAfterWave = 5;

	/** Arena 2 runs to this wave; then comes The Shield. 0 = never. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Rules")
	int32 ShieldAfterWave = 10;

	/** The One-Eyed Liar, after the last wave. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Boss")
	TSubclassOf<AOneEyedLiar> BossClass;

	/** In the endless arena it comes back this often, with this much more health each time. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Endless")
	int32 EndlessBossEvery = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Endless")
	float EndlessBossGrowth = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Boss")
	int32 PointsForBoss = 10000;

	/** For each life left when it falls. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Boss")
	int32 PointsPerLife = 1000;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Boss", meta = (Units = "s"))
	float BossIntroDuration = 4.2f;

	/** Speed of the shaytaan and dajjal it summons. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Boss")
	float SummonSpeed = 240.f;

	/** Seconds between power-ups that appear by themselves during the fight. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Boss", meta = (Units = "s"))
	float BossPickupInterval = 16.f;

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
	void StartRun();
	/** Picks the run up again at a wave (back from The Path). */
	void ContinueRun(int32 FromWaveIndex);
	void EnterMenu(bool bInstantCamera);
	void BeginWaveIntro();
	void BeginBossIntro();
	void BeginBossFight();
	void HandleBossDefeated();
	void HandleBossGone();
	void HandleBossStage(int32 Stage);
	void SummonForBoss(int32 Shaytaan, int32 Dajjal);
	void DropBossPickup();
	bool IsBossWave() const;
	/** This wave's make-up; false past the last one. In the endless arena there is always another. */
	bool FindWave(int32 Index, FArenaWave& OutWave) const;
	FArenaWave CurrentWave;
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

	UPROPERTY(Transient)
	TObjectPtr<AOneEyedLiar> Boss;

	FTimerHandle BossPickupTimer;
	int32 BossBonus = 0;

	EArenaPhase Phase = EArenaPhase::Menu;
	float PhaseStartTime = 0.f;
	int32 WaveIndex = 0;
	FTimerHandle PhaseTimer;
};
