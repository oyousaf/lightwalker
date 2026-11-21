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

	/** Side-to-side rock of the imported model's waddle, in degrees each way. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float WaddleRock = 9.f;

	/** Forward hunch while chasing, in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float WaddleLean = 8.f;

	/** How much the model squashes as it lands each hop (0.1 = 10% shorter). */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float LandingSquash = 0.1f;

	/** Gap between trail blobs; the trail is as long as TrailSegments * TrailSpacing. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float TrailSpacing = 5.5f;

	/** How quickly the trail shrinks back into the body when the devil stops. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float TrailRetractRate = 4.f;

	/** How far the tip of the trail wavers side to side, so it reads as smoke. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float TrailWobble = 9.f;

private:
	void UpdateTrail(float DeltaSeconds);

	/** Blobs that follow each other behind the devil, shrinking and dimming towards the end. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TrailSegments;

	/** The chain the trail follows (before the wobble), and each wisp's base size. */
	TArray<FVector> TrailPoints;
	TArray<float> TrailScales;
};
