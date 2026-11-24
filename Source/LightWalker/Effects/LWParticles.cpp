#include "Effects/LWParticles.h"

#include "Core/LWNeon.h"
#include "Engine/World.h"
#include "Scalability.h"

// ---------------------------------------------------------------------------------------------------------------
// Particle component

ULWParticleComponent::ULWParticleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Particles only glow (through bloom): as solid objects in the indirect lighting, bursts left dark, blotchy ghosts.
	LWNeon::GlowOnly(this);
	bAffectIndirectLightingWhileHidden = false;
	bCastDynamicShadow = false;
	// World-space particles: the component stays at the origin whatever its owner does.
	LWNeon::SetWorldPlaced(this);
}

void ULWParticleComponent::Setup(ELWShape Shape, const FLinearColor& Color, float Intensity)
{
	SetStaticMesh(LWNeon::Shape(Shape));
	SetWorldTransform(FTransform::Identity);
	LWNeon::ApplyGlow(this, Color, Intensity);
}

float ULWParticleComponent::GetQualityScale()
{
	static const float Scales[] = { 0.4f, 0.7f, 1.f, 1.f, 1.f };
	return Scales[FMath::Clamp(Scalability::GetQualityLevels().EffectsQuality, 0, 4)];
}

void ULWParticleComponent::Emit(const FLWEmit& Params)
{
	const int32 Count = FMath::Max(1, FMath::RoundToInt(Params.Count * GetQualityScale()));
	const FVector Axis = Params.Direction.GetSafeNormal();

	for (int32 Index = 0; Index < Count; ++Index)
	{
		FVector Direction;
		if (Axis.IsNearlyZero())
		{
			// Mostly along the arena plane, so bursts read from the top-down camera.
			Direction = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-0.35f, 0.6f)).GetSafeNormal();
		}
		else
		{
			Direction = FMath::VRandCone(Axis, FMath::DegreesToRadians(Params.Spread));
		}

		FVector Offset = FVector::ZeroVector;
		if (Params.OriginRadius > 0.f)
		{
			const FVector2D Disc = FMath::RandPointInCircle(Params.OriginRadius);
			Offset = FVector(Disc.X, Disc.Y, 0.f);
		}

		FParticle& Particle = Particles.AddDefaulted_GetRef();
		Particle.Position = Params.Origin + Offset;
		Particle.Velocity = Direction * FMath::FRandRange(Params.Speed.X, Params.Speed.Y) + FVector(0.f, 0.f, Params.Lift);
		Particle.Rotation = Params.bSpin ? FRotator(FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f)).Quaternion() : FQuat::Identity;
		Particle.Spin = Params.bSpin ? FMath::VRand() * FMath::FRandRange(4.f, 12.f) : FVector::ZeroVector;
		Particle.Age = 0.f;
		Particle.Life = FMath::FRandRange(Params.Life.X, Params.Life.Y);
		Particle.Size = FMath::FRandRange(Params.Size.X, Params.Size.Y);
		Particle.EndSize = Params.EndSize;
		Particle.Drag = Params.Drag;
		Particle.Gravity = Params.Gravity;
		Particle.bFlash = Params.bFlash;
	}
}

void ULWParticleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TArray<FTransform> Transforms;
	Transforms.Reserve(Particles.Num());

	for (int32 Index = Particles.Num() - 1; Index >= 0; --Index)
	{
		FParticle& Particle = Particles[Index];
		Particle.Age += DeltaTime;
		if (Particle.Age >= Particle.Life)
		{
			Particles.RemoveAtSwap(Index, EAllowShrinking::No);
			continue;
		}

		Particle.Velocity *= FMath::Max(0.f, 1.f - Particle.Drag * DeltaTime);
		Particle.Velocity.Z -= Particle.Gravity * DeltaTime;
		Particle.Position += Particle.Velocity * DeltaTime;
		if (!Particle.Spin.IsNearlyZero())
		{
			Particle.Rotation = FQuat(Particle.Spin.GetSafeNormal(), Particle.Spin.Size() * DeltaTime) * Particle.Rotation;
		}
	}

	for (const FParticle& Particle : Particles)
	{
		const float T = Particle.Age / Particle.Life;
		const float Scale = Particle.bFlash
			? Particle.Size * FMath::Sin(PI * FMath::Min(T * 1.4f, 1.f))
			: Particle.Size * FMath::Lerp(1.f, Particle.EndSize, T * T);
		Transforms.Emplace(Particle.Rotation, Particle.Position, FVector(FMath::Max(Scale, 0.001f)));
	}

	// Rebuild the instances to match the live particles.
	if (GetInstanceCount() != Transforms.Num())
	{
		ClearInstances();
		if (Transforms.Num() > 0)
		{
			AddInstances(Transforms, false, true);
		}
	}
	else if (Transforms.Num() > 0)
	{
		BatchUpdateInstancesTransforms(0, Transforms, true, true, true);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// One-shot burst actor

ALWBurst::ALWBurst()
{
	PrimaryActorTick.bCanEverTick = true;
	Particles = CreateDefaultSubobject<ULWParticleComponent>(TEXT("Particles"));
	RootComponent = Particles;
}

void ALWBurst::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (Age > 0.1f && !Particles->HasLiveParticles())
	{
		Destroy();
	}
}

// ---------------------------------------------------------------------------------------------------------------
// The game's effects

void LWFX::Burst(UWorld* World, ELWShape Shape, const FLinearColor& Color, float Intensity, std::initializer_list<FLWEmit> Batches)
{
	if (!World || Batches.size() == 0)
	{
		return;
	}
	ALWBurst* Burst = World->SpawnActor<ALWBurst>();
	if (!Burst)
	{
		return;
	}
	Burst->GetParticles()->Setup(Shape, Color, Intensity);
	for (const FLWEmit& Batch : Batches)
	{
		Burst->GetParticles()->Emit(Batch);
	}
}

void LWFX::HitSpark(UWorld* World, const FVector& Location, const FVector& ShotDirection)
{
	FLWEmit Sparks;
	Sparks.Count = 7;
	Sparks.Origin = Location;
	Sparks.Direction = -ShotDirection;
	Sparks.Spread = 55.f;
	Sparks.Speed = FVector2D(350.f, 750.f);
	Sparks.Life = FVector2D(0.12f, 0.25f);
	Sparks.Size = FVector2D(0.035f, 0.06f);
	Sparks.Drag = 6.f;
	Burst(World, ELWShape::Sphere, FLinearColor(0.7f, 0.95f, 1.f), 14.f, { Sparks });
}

void LWFX::ShaytaanDeath(UWorld* World, const FVector& Location)
{
	FLWEmit Flash;
	Flash.Count = 1;
	Flash.Origin = Location;
	Flash.Speed = FVector2D::ZeroVector;
	Flash.Life = FVector2D(0.16f, 0.16f);
	Flash.Size = FVector2D(1.1f, 1.1f);
	Flash.bFlash = true;

	FLWEmit Embers;
	Embers.Count = 18;
	Embers.Origin = Location;
	Embers.Speed = FVector2D(220.f, 620.f);
	Embers.Life = FVector2D(0.35f, 0.7f);
	Embers.Size = FVector2D(0.05f, 0.11f);
	Embers.Drag = 3.5f;
	Embers.Gravity = -60.f;  // embers drift up

	Burst(World, ELWShape::Sphere, FLinearColor(1.f, 0.18f, 0.08f), 10.f, { Flash, Embers });
}

void LWFX::DajjalHit(UWorld* World, const FVector& Location)
{
	FLWEmit Sparks;
	Sparks.Count = 8;
	Sparks.Origin = Location + FVector(0.f, 0.f, 20.f);
	Sparks.Speed = FVector2D(250.f, 550.f);
	Sparks.Life = FVector2D(0.15f, 0.3f);
	Sparks.Size = FVector2D(0.04f, 0.07f);
	Sparks.Drag = 5.f;
	Burst(World, ELWShape::Sphere, FLinearColor(1.f, 0.8f, 0.3f), 14.f, { Sparks });
}

void LWFX::DajjalDeath(UWorld* World, const FVector& Location)
{
	FLWEmit Flash;
	Flash.Count = 1;
	Flash.Origin = Location;
	Flash.Speed = FVector2D::ZeroVector;
	Flash.Life = FVector2D(0.22f, 0.22f);
	Flash.Size = FVector2D(1.6f, 1.6f);
	Flash.bFlash = true;

	FLWEmit Shards;
	Shards.Count = 24;
	Shards.Origin = Location;
	Shards.Speed = FVector2D(300.f, 800.f);
	Shards.Life = FVector2D(0.5f, 0.9f);
	Shards.Size = FVector2D(0.06f, 0.14f);
	Shards.Drag = 2.5f;
	Shards.Gravity = 900.f;
	Shards.Lift = 250.f;
	Shards.bSpin = true;

	Burst(World, ELWShape::Sphere, FLinearColor(1.f, 0.85f, 0.4f), 12.f, { Flash });
	Burst(World, ELWShape::Cube, FLinearColor(1.f, 0.72f, 0.2f), 8.f, { Shards });
}

void LWFX::PickupCollect(UWorld* World, const FVector& Location, const FLinearColor& Color, bool bBook)
{
	FLWEmit Ring;
	Ring.Count = bBook ? 28 : 16;
	Ring.Origin = Location + FVector(0.f, 0.f, 20.f);
	Ring.Direction = FVector::UpVector;
	Ring.Spread = 88.f;  // nearly flat: an expanding ring
	Ring.Speed = bBook ? FVector2D(350.f, 500.f) : FVector2D(300.f, 380.f);
	Ring.Life = FVector2D(0.3f, 0.45f);
	Ring.Size = FVector2D(0.05f, 0.08f);
	Ring.Drag = 4.f;

	if (bBook)
	{
		FLWEmit Rise;
		Rise.Count = 22;
		Rise.Origin = Location;
		Rise.OriginRadius = 60.f;
		Rise.Direction = FVector::UpVector;
		Rise.Spread = 15.f;
		Rise.Speed = FVector2D(120.f, 320.f);
		Rise.Life = FVector2D(0.8f, 1.4f);
		Rise.Size = FVector2D(0.04f, 0.08f);
		Rise.Drag = 0.8f;
		Burst(World, ELWShape::Sphere, Color, 14.f, { Ring, Rise });
		return;
	}
	Burst(World, ELWShape::Sphere, Color, 12.f, { Ring });
}

void LWFX::LifeLost(UWorld* World, const FVector& Location)
{
	FLWEmit Flash;
	Flash.Count = 1;
	Flash.Origin = Location;
	Flash.Speed = FVector2D::ZeroVector;
	Flash.Life = FVector2D(0.3f, 0.3f);
	Flash.Size = FVector2D(2.2f, 2.2f);
	Flash.bFlash = true;

	FLWEmit Scatter;
	Scatter.Count = 34;
	Scatter.Origin = Location;
	Scatter.Speed = FVector2D(250.f, 900.f);
	Scatter.Life = FVector2D(0.5f, 1.1f);
	Scatter.Size = FVector2D(0.05f, 0.12f);
	Scatter.Drag = 2.f;

	Burst(World, ELWShape::Sphere, FLinearColor(0.6f, 0.95f, 1.f), 12.f, { Flash, Scatter });
}

void LWFX::Respawn(UWorld* World, const FVector& Location)
{
	FLWEmit Column;
	Column.Count = 24;
	Column.Origin = Location - FVector(0.f, 0.f, 40.f);
	Column.OriginRadius = 55.f;
	Column.Direction = FVector::UpVector;
	Column.Spread = 6.f;
	Column.Speed = FVector2D(250.f, 600.f);
	Column.Life = FVector2D(0.4f, 0.8f);
	Column.Size = FVector2D(0.04f, 0.08f);
	Column.Drag = 1.5f;
	Burst(World, ELWShape::Sphere, FLinearColor(0.6f, 0.95f, 1.f), 12.f, { Column });
}

void LWFX::ShieldBlock(UWorld* World, const FVector& Location)
{
	FLWEmit Ring;
	Ring.Count = 22;
	Ring.Origin = Location;
	Ring.Direction = FVector::UpVector;
	Ring.Spread = 88.f;
	Ring.Speed = FVector2D(450.f, 600.f);
	Ring.Life = FVector2D(0.25f, 0.4f);
	Ring.Size = FVector2D(0.06f, 0.1f);
	Ring.Drag = 3.f;
	Burst(World, ELWShape::Sphere, FLinearColor(0.3f, 1.f, 0.45f), 12.f, { Ring });
}
