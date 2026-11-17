#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArenaHUD.generated.h"

/**
 * Placeholder canvas HUD for milestone 1: score, wave, lives and the centre banners.
 * Replaced by CommonUI widgets in the polish pass.
 */
UCLASS()
class LIGHTWALKER_API AArenaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	enum class EAlign : uint8 { Left, Centre, Right };

	void DrawLine(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale, EAlign Align);
};
