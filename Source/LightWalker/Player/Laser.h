#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Laser.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/** A bolt of light fired by Lightwalker. Flies straight and hits the first enemy it touches (or all of them, piercing). */
UCLASS()
class LIGHTWALKER_API ALaser : public AActor
{
	GENERATED_BODY()

public:
	ALaser();

	virtual void Tick(float DeltaSeconds) override;

	/** Sets the direction of travel and the area it is destroyed on leaving. */
	void Launch(const FVector& InDirection, const FBox2D& InBounds);

	/**
	 * Power-up tweaks, applied straight after spawning: Size scales the bolt and its hit radius, SpeedScale its speed.
	 * A piercing bolt hits every enemy in its path once instead of stopping at the first.
	 */
	void SetPower(float Size, float SpeedScale, bool bInPiercing, const FLinearColor& InColor);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<UStaticMeshComponent> Beam;

	/** 2014: BULLETSPEED of 10 px per frame at 24 fps, scaled 5 units per pixel. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float Speed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	FLinearColor Color = FLinearColor(0.75f, 0.95f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float GlowIntensity = 12.f;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	FVector Direction = FVector::ForwardVector;
	FBox2D Bounds = FBox2D(ForceInit);
	bool bSpent = false;
	bool bPiercing = false;

	/** Enemies a piercing bolt has already hit. */
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
