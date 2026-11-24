#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "GameFramework/Actor.h"
#include "LWParticles.generated.h"

/** How a batch of particles is thrown out. Ranges are (min, max), picked at random per particle. */
struct FLWEmit
{
	int32 Count = 10;
	FVector Origin = FVector::ZeroVector;
	/** Main direction; zero throws them evenly in every direction (flattened towards the arena plane). */
	FVector Direction = FVector::ZeroVector;
	/** Cone half-angle around Direction, in degrees. */
	float Spread = 180.f;
	FVector2D Speed = FVector2D(200.f, 500.f);
	FVector2D Life = FVector2D(0.3f, 0.6f);
	/** Uniform scale of the 100-unit basic shape. */
	FVector2D Size = FVector2D(0.06f, 0.12f);
	/** Size at the end of life as a fraction of the start. */
	float EndSize = 0.f;
	/** Speed lost per second (0-1 of current speed, roughly). */
	float Drag = 3.f;
	/** Downward pull in cm/s^2 (negative floats them up). */
	float Gravity = 0.f;
	/** Extra upward kick added to every particle. */
	float Lift = 0.f;
	/** Grow then shrink over the lifetime (a flash), instead of shrinking from the start. */
	bool bFlash = false;
	/** Tumble as they fly (for shards). */
	bool bSpin = false;
	/** Spread start positions over a disc of this radius around Origin. */
	float OriginRadius = 0.f;
};

/**
 * A pool of glowing particles drawn as instances of one basic shape in one neon colour. Particles live in world
 * space, so the owner can move without dragging them along. Counts scale with the Effects quality setting.
 */
UCLASS()
class LIGHTWALKER_API ULWParticleComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	ULWParticleComponent();

	/** Shape, colour and glow strength. Call before emitting. */
	void Setup(ELWShape Shape, const FLinearColor& Color, float Intensity);

	void Emit(const FLWEmit& Params);

	bool HasLiveParticles() const { return Particles.Num() > 0; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Particle count multiplier for the current Effects quality (Low 0.4 ... Epic 1). */
	static float GetQualityScale();

private:
	struct FParticle
	{
		FVector Position;
		FVector Velocity;
		FQuat Rotation;
		FVector Spin;
		float Age;
		float Life;
		float Size;
		float EndSize;
		float Drag;
		float Gravity;
		bool bFlash;
	};

	TArray<FParticle> Particles;
};

/** A one-shot burst: spawns, throws its particles, and removes itself once they're gone. */
UCLASS()
class LIGHTWALKER_API ALWBurst : public AActor
{
	GENERATED_BODY()

public:
	ALWBurst();

	virtual void Tick(float DeltaSeconds) override;

	ULWParticleComponent* GetParticles() const { return Particles; }

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULWParticleComponent> Particles;

	float Age = 0.f;
};

/** The game's effects, one call each. */
namespace LWFX
{
	/** Spawns a burst actor of the given look and throws one or more batches from it. */
	LIGHTWALKER_API void Burst(UWorld* World, ELWShape Shape, const FLinearColor& Color, float Intensity, std::initializer_list<FLWEmit> Batches);

	LIGHTWALKER_API void HitSpark(UWorld* World, const FVector& Location, const FVector& ShotDirection);
	LIGHTWALKER_API void ShaytaanDeath(UWorld* World, const FVector& Location);
	LIGHTWALKER_API void DajjalHit(UWorld* World, const FVector& Location);
	LIGHTWALKER_API void DajjalDeath(UWorld* World, const FVector& Location);
	LIGHTWALKER_API void PickupCollect(UWorld* World, const FVector& Location, const FLinearColor& Color, bool bBook);
	LIGHTWALKER_API void LifeLost(UWorld* World, const FVector& Location);
	LIGHTWALKER_API void Respawn(UWorld* World, const FVector& Location);
	LIGHTWALKER_API void ShieldBlock(UWorld* World, const FVector& Location);
}
