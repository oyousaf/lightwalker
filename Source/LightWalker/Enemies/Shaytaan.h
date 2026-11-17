#pragma once

#include "CoreMinimal.h"
#include "Enemies/EnemyBase.h"
#include "Shaytaan.generated.h"

/** Chaser: heads straight for Lightwalker. The game mode speeds the pack up as it thins out. */
UCLASS()
class LIGHTWALKER_API AShaytaan : public AEnemyBase
{
	GENERATED_BODY()

public:
	AShaytaan();

protected:
	virtual void UpdateMovement(float DeltaSeconds) override;
};
