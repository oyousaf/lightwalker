#pragma once

#include "CoreMinimal.h"
#include "Enemies/EnemyBase.h"
#include "Shaytaan.generated.h"

/**
 * Chaser: a little devil that heads straight for Lightwalker, bobbing as it goes and leaving a fading trail.
 * The game mode speeds the pack up as it thins out.
 */
UCLASS()
class LIGHTWALKER_API AShaytaan : public AEnemyBase
{
	GENERATED_BODY()

public:
	AShaytaan();

protected:
	virtual void BeginPlay() override;
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

	/** Gap between trail blobs; the trail is as long as TrailSegments * TrailSpacing. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float TrailSpacing = 5.5f;

	/** How quickly the trail shrinks back into the body when the devil stops. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float TrailRetractRate = 4.f;

private:
	void UpdateTrail(float DeltaSeconds);

	/** Blobs that follow each other behind the devil, shrinking and dimming towards the end. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TrailSegments;
};
