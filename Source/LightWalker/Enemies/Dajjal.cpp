#include "Enemies/Dajjal.h"

#include "Effects/LWParticles.h"
#include "Player/LightWalkerPawn.h"

ADajjal::ADajjal()
{
	ModelPath = FSoftObjectPath(TEXT("/Game/Enemies/SM_Dajjal.SM_Dajjal"));
	// This model's eye is on the far side from the others' faces.
	ModelYaw = 90.f;
	HitPoints = 3;
	DeathSound = ELWSound::DajjalDeath;
}

void ADajjal::InitEnemy(ALightWalkerPawn* InPlayer, const FBox2D& InBounds, float InMoveSpeed)
{
	Super::InitEnemy(InPlayer, InBounds, InMoveSpeed);
	PickDirection();
}

void ADajjal::PickDirection()
{
	static const FVector Directions[] = { FVector::ForwardVector, FVector::BackwardVector, FVector::RightVector, FVector::LeftVector };
	Direction = Directions[FMath::RandRange(0, UE_ARRAY_COUNT(Directions) - 1)];
	RedirectRemaining = FMath::FRandRange(RedirectInterval.X, RedirectInterval.Y);
}

void ADajjal::UpdateMovement(float DeltaSeconds)
{
	RedirectRemaining -= DeltaSeconds;
	if (RedirectRemaining <= 0.f)
	{
		PickDirection();
	}

	FVector Location = GetActorLocation() + Direction * MoveSpeed * DeltaSeconds;

	if (Bounds.bIsValid)
	{
		if (Location.X <= Bounds.Min.X || Location.X >= Bounds.Max.X)
		{
			Direction.X = -Direction.X;
		}
		if (Location.Y <= Bounds.Min.Y || Location.Y >= Bounds.Max.Y)
		{
			Direction.Y = -Direction.Y;
		}
		Location.X = FMath::Clamp(Location.X, Bounds.Min.X, Bounds.Max.X);
		Location.Y = FMath::Clamp(Location.Y, Bounds.Min.Y, Bounds.Max.Y);
	}

	SetActorLocation(Location);
}

void ADajjal::UpdateLook(float DeltaSeconds)
{
	// The whole pyramid turns so its eye keeps watching Lightwalker, even during the wave intro.
	if (const ALightWalkerPawn* LightWalker = GetPlayer())
	{
		TurnVisualTowards(LightWalker->GetActorLocation() - GetActorLocation(), DeltaSeconds, WatchTurnRate);
	}

	Visual->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(LookTime * 2.f) * 5.f));
}

void ADajjal::SpawnDeathEffect()
{
	LWFX::DajjalDeath(GetWorld(), GetActorLocation());
}
