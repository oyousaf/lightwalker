#pragma once

#include "CoreMinimal.h"

class AArenaGameMode;
class ALightWalkerPawn;

#if !UE_BUILD_SHIPPING
/** Dev only (-LWAutoFight): plays the arena by itself, to test waves and the boss. */
namespace LWAutoPlay
{
	/** Aims at the most pressing enemy and steps wherever is safest. Call every frame. */
	void Fight(AArenaGameMode& GameMode, ALightWalkerPawn* LightWalker);
}
#endif
