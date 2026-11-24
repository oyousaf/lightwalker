#pragma once

#include "CoreMinimal.h"
#include "UI/ArenaHUD.h"
#include "ShieldHUD.generated.h"

/** The Shield's HUD: which wall, and how much of it stands. */
UCLASS()
class LIGHTWALKER_API AShieldHUD : public AArenaHUD
{
	GENERATED_BODY()

protected:
	virtual bool IsShown() const override;
	virtual void DrawTopCentre(const FLayout& Layout) override;
	virtual FBanner GetBanner() const override;
	virtual float GetMessageHeight() const override { return 0.5f; }
};
