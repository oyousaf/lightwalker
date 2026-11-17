#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Laser.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/** A bolt of light fired by Lightwalker. Flies straight, kills the first enemy it touches. */
UCLASS()
class LIGHTWALKER_API ALaser : public AActor
{
	GENERATED_BODY()

public:
	ALaser();

	virtual void Tick(float DeltaSeconds) override;

	/** Sets the direction of travel and the area it is destroyed on leaving. */
	void Launch(const FVector& InDirection, const FBox2D& InBounds);

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
};
