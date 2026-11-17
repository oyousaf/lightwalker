#pragma once

#include "CoreMinimal.h"
#include "Enemies/EnemyBase.h"
#include "Dajjal.generated.h"

class UProceduralMeshComponent;

/**
 * Wanderer: a golden pyramid with a single Horus-style eye that turns to watch Lightwalker wherever they go.
 * Drifts in one of the four directions, bounces off the arena edge, and changes its mind every few seconds.
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
	virtual void BeginPlay() override;
	virtual void UpdateMovement(float DeltaSeconds) override;
	virtual void UpdateLook(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<UProceduralMeshComponent> Pyramid;

	/** Sits on the pyramid's front face: +X points out of the face, +Z up the slope. */
	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USceneComponent> EyePivot;

	/** Seconds between random changes of direction, picked between min and max. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	FVector2D RedirectInterval = FVector2D(1.5f, 3.5f);

	/** How quickly the eye turns to follow Lightwalker. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float WatchTurnRate = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	FLinearColor PyramidColor = FLinearColor(1.f, 0.55f, 0.02f);

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	FLinearColor MarkingColor = FLinearColor(1.f, 0.78f, 0.2f);

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	FLinearColor EyeWhiteColor = FLinearColor(1.f, 0.95f, 0.8f);

private:
	void BuildPyramid();
	void PickDirection();

	FVector Direction = FVector::ForwardVector;
	float RedirectRemaining = 0.f;
};
