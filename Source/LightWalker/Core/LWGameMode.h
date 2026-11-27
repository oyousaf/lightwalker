#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LWGameMode.generated.h"

class AEnemyBase;
class ALightWalkerPawn;
class ALWWeather;
class ULightWalkerGameInstance;
class APickup;
enum class EPickupType : uint8;

/**
 * What every mode (Arena, The Path, The Shield) has in common, so the pawn, enemies, menus and game feel don't need to
 * know which one they're in. A run moves between modes by travelling between maps; the run itself (score, lives,
 * upgrades) lives on ULightWalkerGameInstance.
 */
UCLASS(Abstract)
class LIGHTWALKER_API ALWGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;

	virtual void HandleEnemyKilled(AEnemyBase* Enemy) {}

	/** An enemy (or another hazard) touched Lightwalker. */
	virtual void HandlePlayerTouched(ALightWalkerPawn* LightWalker) {}

	/** Confirm pressed: starts a new run once this one is over. */
	virtual void RequestRestart() {}

	/** Starts again from wave 1 (pause menu). */
	virtual void RestartRun() {}
	virtual void ReturnToMenu() {}
	virtual void StartFromMenu() {}

	virtual bool IsRunOver() const { return false; }
	virtual bool IsInMenu() const { return false; }

	/** How far the run has got, for the leaderboard. */
	virtual FString GetReached() const { return FString(); }

	/** The season changed in Settings. */
	virtual void RefreshSeason();

	/** Shakes this mode's camera (0-1). */
	virtual void AddCameraTrauma(float Amount) {}

	/** True when a bolt at this point has hit something solid. */
	virtual bool BlocksShot(const FVector& Location) const { return false; }

	/** Whether Lightwalker's bolts can be fired now (not while a wave or stage is still being introduced). */
	virtual bool CanFire() const { return true; }

	/** Whether the jump control does anything here (The Path). */
	virtual bool UsesJump() const { return false; }

protected:
	ULightWalkerGameInstance* GetRun() const;
	ALightWalkerPawn* GetLightWalker() const;

	/** Fills a box of air with the season's weather (one that travels with the camera is measured from it). */
	void SetupWeather(const FBox& Air, bool bFollowCamera = false);

	/** Lightwalker's shield takes the hit, if it's up (with its sound and flash). */
	bool BlockWithShield(ALightWalkerPawn* LightWalker);

	APickup* SpawnPickup(EPickupType Type, const FVector& Where);
	/** Any power-up (not the book). */
	static EPickupType RandomPowerUp();

	/** The run is over: after a moment for the banner, the name entry or the leaderboard comes up. */
	void EndRun(float Delay);
	/** Straight to it (confirm pressed), or not at all (a new run). */
	void ShowRunEnd();
	void CancelRunEnd();

	/** Fades to black and loads another mode's map. */
	void TravelTo(const TCHAR* Map, const TCHAR* GameMode);
	static bool MapExists(const TCHAR* Map);
	void FadeIn();

private:
	UPROPERTY(Transient)
	TObjectPtr<ALWWeather> Weather;

	FString TravelMap;
	FString TravelOptions;
	FTimerHandle TravelTimer;
	FTimerHandle RunEndTimer;

#if !UE_BUILD_SHIPPING
	/** Dev only: -LWShots=N takes N high-res screenshots, three seconds apart (or -LWShotEvery=S), then quits. */
	void TakeDevScreenshot();
	FTimerHandle DevShotTimer;
#endif
};

namespace LWMaps
{
	inline const TCHAR* const Arena = TEXT("/Game/Maps/L_Arena");
	inline const TCHAR* const ArenaMode = TEXT("/Script/LightWalker.ArenaGameMode");
	inline const TCHAR* const Path = TEXT("/Game/Maps/L_Path");
	inline const TCHAR* const PathMode = TEXT("/Script/LightWalker.PathGameMode");
	inline const TCHAR* const Shield = TEXT("/Game/Maps/L_Shield");
	inline const TCHAR* const ShieldMode = TEXT("/Script/LightWalker.ShieldGameMode");
}
