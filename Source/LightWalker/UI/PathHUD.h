#pragma once

#include "CoreMinimal.h"
#include "UI/ArenaHUD.h"
#include "PathHUD.generated.h"

/** The Path's HUD: how much light is left and how far along the path, in place of the wave. */
UCLASS()
class LIGHTWALKER_API APathHUD : public AArenaHUD
{
	GENERATED_BODY()

protected:
	virtual bool IsShown() const override;
	virtual void DrawTopCentre(const FLayout& Layout) override;
	virtual FBanner GetBanner() const override;
};
