#include "Player/Laser.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWGameMode.h"
#include "Core/LWNeon.h"
#include "Effects/LWParticles.h"
#include "Enemies/EnemyBase.h"

ALaser::ALaser()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = 5.f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(18.f);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Collision->SetGenerateOverlapEvents(true);
	RootComponent = Collision;

	// The cylinder runs along +Z; pitch it so its length runs along the actor's +X (direction of travel).
	Beam = LWNeon::MakeMesh(this, TEXT("Beam"), ELWShape::Cylinder, Collision);
	Beam->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Beam->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.7f));
}

void ALaser::BeginPlay()
{
	Super::BeginPlay();

	LWNeon::ApplyGlow(Beam, Color, GlowIntensity);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ALaser::HandleOverlap);
}

void ALaser::Launch(const FVector& InDirection, const FBox2D& InBounds)
{
	Direction = InDirection.GetSafeNormal();
	Bounds = InBounds;
	SetActorRotation(Direction.Rotation());
}

void ALaser::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector Location = GetActorLocation() + Direction * Speed * DeltaSeconds;
	SetActorLocation(Location);

	// Leaving the arena on any side ends the laser (the 2014 left-firing laser checked the wrong axis here).
	if (Bounds.bIsValid && !Bounds.IsInside(FVector2D(Location.X, Location.Y)))
	{
		Destroy();
		return;
	}

	// Solid ground stops a bolt (The Path).
	const ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>();
	if (GameMode && GameMode->BlocksShot(Location))
	{
		LWFX::HitSpark(GetWorld(), Location, Direction);
		Destroy();
	}
}

void ALaser::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AEnemyBase* Enemy = Cast<AEnemyBase>(OtherActor);
	if (bSpent || !Enemy || Enemy->IsDead())
	{
		return;
	}

	LWFX::HitSpark(GetWorld(), GetActorLocation(), Direction);

	if (bPiercing)
	{
		bool bAlreadyHit = false;
		HitActors.Add(Enemy, &bAlreadyHit);
		if (!bAlreadyHit)
		{
			Enemy->TakeHit();
		}
		return;
	}

	bSpent = true;
	Enemy->TakeHit();
	Destroy();
}

void ALaser::SetPower(float Size, float SpeedScale, bool bInPiercing, const FLinearColor& InColor)
{
	Speed *= SpeedScale;
	bPiercing = bInPiercing;
	Color = InColor;

	Collision->SetSphereRadius(Collision->GetUnscaledSphereRadius() * Size);
	FVector BeamScale = Beam->GetRelativeScale3D();
	BeamScale.X *= Size;
	BeamScale.Y *= Size;
	BeamScale.Z *= bPiercing ? 1.6f : FMath::Max(1.f, Size * 0.8f);
	Beam->SetRelativeScale3D(BeamScale);
	LWNeon::ApplyGlow(Beam, Color, GlowIntensity);
}
