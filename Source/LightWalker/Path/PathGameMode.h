#pragma once

#include "CoreMinimal.h"
#include "Core/LWGameMode.h"
#include "PathGameMode.generated.h"

class APathCourse;
class APathOrb;
class APathPawn;

UENUM(BlueprintType)
enum class EPathPhase : uint8
{
	/** The banner is up and Lightwalker waits at the start. */
	Intro,
	Running,
	/** A life was lost; back to the last lantern shortly. */
	Down,
	/** Through the gate of light. */
	Finished,
	GameOver,
	/** Fading out to the arena. */
	Leaving
};

/**
 * The Path, the stage between Arena 1 and Arena 2 (from the 2014 Green Ball). Run and jump to the gate of light before
 * the lantern goes out: it fades all the time, and orbs of light refill it. Spikes, falls, enemies and the dark each
 * cost a life and send Lightwalker back to the last lantern passed. Carries on the arena's run.
 */
UCLASS()
class LIGHTWALKER_API APathGameMode : public ALWGameMode
{
	GENERATED_BODY()

public:
	APathGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	virtual void HandleEnemyKilled(AEnemyBase* Enemy) override;
	virtual void HandlePlayerTouched(ALightWalkerPawn* LightWalker) override;
	virtual void RequestRestart() override;
	virtual void RestartRun() override;
	virtual void ReturnToMenu() override;
	virtual bool IsRunOver() const override { return Phase == EPathPhase::GameOver; }
	virtual void AddCameraTrauma(float Amount) override;
	virtual bool BlocksShot(const FVector& Location) const override;
	virtual bool UsesJump() const override { return true; }

	EPathPhase GetPhase() const { return Phase; }
	float GetPhaseAge() const;

	/** 0-1. */
	float GetLight() const { return Light; }
	float GetProgress() const;
	/** Where the checkpoints fall along the course, 0-1, and how many have been lit. */
	TArray<float> GetCheckpointMarks() const;
	int32 GetCheckpointsLit() const { return Checkpoint; }
	bool WentDark() const { return bWentDark; }
	int32 GetFinishBonus() const { return FinishBonus; }

protected:
	/** Seconds for a full lantern to go out. Book upgrades to the light make it last longer. */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Light", meta = (Units = "s"))
	float LightDuration = 15.f;

	/** How much of the lantern one orb refills. */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Light")
	float OrbLight = 0.14f;

	/** Below this the lantern gutters: a warning sound and a pulsing bar. */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Light")
	float LowLight = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules")
	int32 StartingLives = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules")
	int32 PointsPerOrb = 50;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules")
	int32 PointsPerShaytaan = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules")
	int32 PointsPerDajjal = 300;

	/** For reaching the gate, plus this much again for a full lantern. */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules")
	int32 FinishPoints = 1000;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules", meta = (Units = "s"))
	float IntroDuration = 2.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules", meta = (Units = "s"))
	float RespawnDelay = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules", meta = (Units = "s"))
	float RespawnGrace = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules", meta = (Units = "s"))
	float FinishDuration = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Rules", meta = (Units = "s"))
	float RestartLockout = 1.f;

private:
	APathPawn* GetPathPawn() const;
	void SetPhase(EPathPhase NewPhase, float Duration = 0.f, void (APathGameMode::*Next)() = nullptr);
	void BeginRunning();
	void Respawn();
	void LoseLife(const FVector& Location);
	void Finish();
	void LeaveForArena();
	void SpawnField();
	void SetEnemiesActive(bool bActive);
	void UpdateLight(float DeltaSeconds);
	void UpdateCheckpoints();

	UPROPERTY(Transient)
	TObjectPtr<APathCourse> Course;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APathOrb>> Orbs;

	EPathPhase Phase = EPathPhase::Intro;
	float PhaseStart = 0.f;
	FTimerHandle PhaseTimer;

	float Light = 1.f;
	float LowestLight = 1.f;
	float WarningTimer = 0.f;
	int32 Checkpoint = 0;
	int32 FinishBonus = 0;
	bool bWentDark = false;
};
