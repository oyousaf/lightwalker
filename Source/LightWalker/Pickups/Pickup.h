#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class ALightWalkerPawn;
class ULWParticleComponent;
class UPointLightComponent;
class USceneComponent;
class UTexture2D;

UENUM(BlueprintType)
enum class EPickupType : uint8
{
	/** Timed: a bigger light, bigger bolts and a faster fire rate. */
	LightOrb,
	/** Timed: three bolts in a fan. */
	SpreadShot,
	/** Timed: bolts pass through everything in their path. */
	PiercingBeam,
	/** Lasts until it blocks one hit. */
	Shield,
	/** Rare: a lasting upgrade for the rest of the run, and a line of wisdom. */
	Book,

	Count UMETA(Hidden)
};

namespace LWPickups
{
	/** Display name, as shown on the HUD. */
	LIGHTWALKER_API FString GetName(EPickupType Type);
	LIGHTWALKER_API FLinearColor GetColor(EPickupType Type);
	/** Its icon, on the HUD and as the pickup itself. Null if the art hasn't been imported. */
	LIGHTWALKER_API UTexture2D* GetIcon(EPickupType Type);
}

/**
 * Something dropped by a killed enemy. It floats where it fell, facing the camera, blinks out after a while,
 * and is collected by Lightwalker walking over it.
 */
UCLASS()
class LIGHTWALKER_API APickup : public AActor
{
	GENERATED_BODY()

public:
	APickup();

	virtual void Tick(float DeltaSeconds) override;

	/** Called straight after spawning. Builds the look for this type. */
	void InitPickup(EPickupType InType, ALightWalkerPawn* InPlayer);

	EPickupType GetType() const { return Type; }

	/** Placed in a level rather than dropped: it waits to be found instead of blinking out. */
	void SetPersistent() { bPersistent = true; }

	/** Drifts across the play plane (on The Shield they fall towards Lightwalker), and is easier to catch. */
	void SetDrift(const FVector& Velocity, float InCollectRadius) { Drift = Velocity; CollectRadius = InCollectRadius; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USceneComponent> Root;

	/** Bobs, sways and faces the camera; the icon hangs off this. */
	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USceneComponent> Visual;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<UPointLightComponent> Light;

	/** Little motes drifting up off the pickup. */
	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<ULWParticleComponent> Sparkles;

	/** Seconds before it disappears. It blinks for the last BlinkTime of them. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float Lifetime = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float BlinkTime = 3.f;

	/** How close Lightwalker's centre has to come to collect it. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float CollectRadius = 80.f;

	/** The icon's width and height. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float IconSize = 125.f;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float IconGlow = 2.f;

private:
	TWeakObjectPtr<ALightWalkerPawn> Player;
	EPickupType Type = EPickupType::LightOrb;
	bool bPersistent = false;
	FVector Drift = FVector::ZeroVector;
	float Age = 0.f;
	float SparkleTimer = 0.f;
	float LightIntensity = 12.f;
	float BlinkPhase = 0.f;
};
