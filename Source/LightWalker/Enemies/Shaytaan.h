#pragma once

#include "CoreMinimal.h"
#include "Enemies/EnemyBase.h"
#include "Shaytaan.generated.h"

/**
 * Chaser: a little devil that heads straight for Lightwalker, bobbing as it goes.
 * The game mode speeds the pack up as it thins out.
 */
UCLASS()
class LIGHTWALKER_API AShaytaan : public AEnemyBase
{
	GENERATED_BODY()

public:
	AShaytaan();

protected:
	virtual void UpdateMovement(float DeltaSeconds) override;
	virtual void UpdateLook(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	FLinearColor BodyColor = FLinearColor(1.f, 0.03f, 0.1f);

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	FLinearColor EyeColor = FLinearColor(1.f, 0.85f, 0.25f);

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float BobHeight = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float BobRate = 7.f;
};
