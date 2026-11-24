#pragma once

#include "CoreMinimal.h"
#include "Core/LWGameMode.h"
#include "ShieldGameMode.generated.h"

class AArena;
class AShieldField;
class AShieldPawn;
struct FWallBlock;

UENUM(BlueprintType)
enum class EShieldPhase : uint8
{
	/** The wall is rising. */
	WallIntro,
	Playing,
	/** A life was lost; another beam shortly. */
	Down,
	WallCleared,
	Finished,
	GameOver,
	/** Fading out to the arena. */
	Leaving
};

/**
 * The Shield, the stage between Arena 2 and Arena 3 (from the 2014 block-breaker). Three walls of darkness: keep your own
 * beam in play off the shield until they break. The walls answer with dark bolts and caged shaytaan, which narrow the
 * shield when it stops them and cost a life when it doesn't. Carries on the arena's run.
 */
UCLASS()
class LIGHTWALKER_API AShieldGameMode : public ALWGameMode
{
	GENERATED_BODY()

public:
	AShieldGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	virtual void HandleEnemyKilled(AEnemyBase* Enemy) override;
	virtual void HandlePlayerTouched(ALightWalkerPawn* LightWalker) override;
	virtual void RequestRestart() override;
	virtual void RestartRun() override;
	virtual void ReturnToMenu() override;
	virtual bool IsRunOver() const override { return Phase == EShieldPhase::GameOver; }
	virtual void AddCameraTrauma(float Amount) override;

	EShieldPhase GetPhase() const { return Phase; }
	int32 GetWallNumber() const { return Wall + 1; }
	int32 GetWallCount() const;
	float GetWallLeft() const;
	bool IsGreatEye() const;
	bool IsBeamHeld() const;
	bool WasBeamLost() const { return bBeamLost; }
	FString GetWallName() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules")
	int32 StartingLives = 3;

	/** For each hit a block took to break. */
	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules")
	int32 PointsPerHit = 20;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules")
	int32 PointsPerShaytaan = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules")
	int32 PointsPerWall = 500;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules")
	int32 PointsForEye = 1500;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules")
	float ShaytaanSpeed = 210.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules")
	float PickupFallSpeed = 290.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules", meta = (Units = "s"))
	float IntroDuration = 2.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules", meta = (Units = "s"))
	float RespawnDelay = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules", meta = (Units = "s"))
	float ClearedDuration = 2.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules", meta = (Units = "s"))
	float FinishDuration = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules", meta = (Units = "s"))
	float RestartLockout = 1.f;

	/** How far up from the bottom of the arena Lightwalker's line is. */
	UPROPERTY(EditDefaultsOnly, Category = "Shield|Rules")
	float LineFromBottom = 190.f;

private:
	AShieldPawn* GetShieldPawn() const;
	FVector GetStart() const;
	void SetPhase(EShieldPhase NewPhase, float Duration = 0.f, void (AShieldGameMode::*Next)() = nullptr);
	void BeginWall();
	void BeginPlaying();
	void NextWall();
	void Respawn();
	void LoseLife(bool bFromBeam);
	void LeaveForArena();
	void HandleBlock(const FWallBlock& Block, bool bBroken);
	void HandleWallCleared();
	void DropPickup(const FVector2D& Where, bool bBook);
	void ClearStrays(bool bPickups);

	UPROPERTY(Transient)
	TObjectPtr<AArena> Arena;

	UPROPERTY(Transient)
	TObjectPtr<AShieldField> Field;

	EShieldPhase Phase = EShieldPhase::WallIntro;
	float PhaseStart = 0.f;
	FTimerHandle PhaseTimer;
	int32 Wall = 0;
	bool bBeamLost = false;
};
