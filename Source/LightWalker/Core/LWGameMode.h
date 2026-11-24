#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LWGameMode.generated.h"

class AEnemyBase;
class ALightWalkerPawn;
class ULightWalkerGameInstance;

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

	/** Shakes this mode's camera (0-1). */
	virtual void AddCameraTrauma(float Amount) {}

	/** True when a bolt at this point has hit something solid. */
	virtual bool BlocksShot(const FVector& Location) const { return false; }

	/** Whether the jump control does anything here (The Path). */
	virtual bool UsesJump() const { return false; }

protected:
	ULightWalkerGameInstance* GetRun() const;
	ALightWalkerPawn* GetLightWalker() const;

	/** Fades to black and loads another mode's map. */
	void TravelTo(const TCHAR* Map, const TCHAR* GameMode);
	static bool MapExists(const TCHAR* Map);
	void FadeIn();

private:
	FString TravelMap;
	FString TravelOptions;
	FTimerHandle TravelTimer;

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
