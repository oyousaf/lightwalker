#include "Enemies/EnemyBase.h"

#include "Arena/ArenaGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Engine/World.h"
#include "Player/LightWalkerPawn.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(Radius);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Collision->SetGenerateOverlapEvents(true);
	RootComponent = Collision;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	Collision->SetSphereRadius(Radius);
	LWNeon::ApplyGlow(Body, Color, GlowIntensity);
}

void AEnemyBase::InitEnemy(ALightWalkerPawn* InPlayer, const FBox2D& InBounds, float InMoveSpeed)
{
	Player = InPlayer;
	Bounds = InBounds;
	MoveSpeed = InMoveSpeed;
}

void AEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (SpinRate != 0.f)
	{
		Body->AddLocalRotation(FRotator(0.f, SpinRate * DeltaSeconds, 0.f));
	}

	if (!bActive || bDead)
	{
		return;
	}

	UpdateMovement(DeltaSeconds);
	CheckPlayerContact();
}

void AEnemyBase::CheckPlayerContact()
{
	// A distance check every frame rather than an overlap event, so an enemy still sitting on the player
	// when their respawn grace runs out still counts as a hit.
	ALightWalkerPawn* LightWalker = Player.Get();
	if (!LightWalker || !LightWalker->IsVulnerable())
	{
		return;
	}

	const float ContactDistance = Radius + LightWalker->GetContactRadius();
	if (FVector::DistSquared2D(GetActorLocation(), LightWalker->GetActorLocation()) <= FMath::Square(ContactDistance))
	{
		if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
		{
			GameMode->HandlePlayerTouched(LightWalker);
		}
	}
}

void AEnemyBase::Kill()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
	{
		GameMode->HandleEnemyKilled(this);
	}
	Destroy();
}
