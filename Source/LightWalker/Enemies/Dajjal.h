#pragma once

#include "CoreMinimal.h"
#include "Enemies/EnemyBase.h"
#include "Dajjal.generated.h"

/**
 * Wanderer: drifts in one of the four directions, bounces off the arena edge, and changes its mind every few seconds.
 * (In 2014 its random direction always came out as "down", and it never changed direction except at a wall.)
 */
UCLASS()
class LIGHTWALKER_API ADajjal : public AEnemyBase
{
	GENERATED_BODY()

public:
	ADajjal();

	virtual void InitEnemy(ALightWalkerPawn* InPlayer, const FBox2D& InBounds, float InMoveSpeed) override;

protected:
	virtual void UpdateMovement(float DeltaSeconds) override;

	/** Seconds between random changes of direction, picked between min and max. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	FVector2D RedirectInterval = FVector2D(1.5f, 3.5f);

private:
	void PickDirection();

	FVector Direction = FVector::ForwardVector;
	float RedirectRemaining = 0.f;
};
