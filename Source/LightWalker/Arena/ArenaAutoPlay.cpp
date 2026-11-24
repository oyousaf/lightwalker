#include "Arena/ArenaAutoPlay.h"

#if !UE_BUILD_SHIPPING

#include "Arena/Arena.h"
#include "Arena/ArenaGameMode.h"
#include "Enemies/OneEyedLiar.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Pickups/Pickup.h"
#include "Player/LightWalkerPawn.h"

void LWAutoPlay::Fight(AArenaGameMode& GameMode, ALightWalkerPawn* LightWalker)
{
	if (!LightWalker || !GameMode.GetArena())
	{
		return;
	}
	if (GameMode.GetPhase() != EArenaPhase::Fighting || LightWalker->IsDowned())
	{
		LightWalker->SetMoveInput(FVector2D::ZeroVector);
		LightWalker->SetAimInput(FVector2D::ZeroVector);
		return;
	}

	const FVector2D Here(LightWalker->GetActorLocation());
	const AOneEyedLiar* Boss = GameMode.GetBoss();

	// Shoot the nearest enemy; the boss when nothing is close, and its guardians before it.
	const AEnemyBase* Target = nullptr;
	float Nearest = TNumericLimits<float>::Max();
	for (const AEnemyBase* Enemy : GameMode.GetEnemies())
	{
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}
		float Distance = FVector2D::Distance(Here, FVector2D(Enemy->GetActorLocation()));
		if (Enemy == Boss)
		{
			Distance = Boss->IsGuarded() ? 1.0e6f : FMath::Max(Distance - 200.f, 520.f);
		}
		if (Distance < Nearest)
		{
			Nearest = Distance;
			Target = Enemy;
		}
	}
	if (Target)
	{
		const FVector2D To = (FVector2D(Target->GetActorLocation()) - Here).GetSafeNormal();
		LightWalker->SetAimInput(FVector2D(To.Y, To.X));
	}

	// Try standing still and sixteen ways to go, looking a little way ahead, and take the safest.
	const FBox2D Bounds = GameMode.GetArena()->GetPlayBounds(60.f);
	const float Speed = 720.f;
	FVector2D Best = FVector2D::ZeroVector;
	float BestCost = TNumericLimits<float>::Max();
	for (int32 Option = -1; Option < 16; ++Option)
	{
		const float Angle = Option * PI / 8.f;
		const FVector2D Way = Option < 0 ? FVector2D::ZeroVector : FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		float Cost = 0.f;
		for (const float Time : { 0.1f, 0.2f, 0.35f, 0.5f, 0.7f })
		{
			FVector2D At = Here + Way * Speed * Time;
			At.X = FMath::Clamp(At.X, Bounds.Min.X, Bounds.Max.X);
			At.Y = FMath::Clamp(At.Y, Bounds.Min.Y, Bounds.Max.Y);
			const float Soon = 1.f / (0.3f + Time);
			for (const AEnemyBase* Enemy : GameMode.GetEnemies())
			{
				if (!Enemy || Enemy->IsDead())
				{
					continue;
				}
				const FVector2D Position(Enemy->GetActorLocation());
				const FVector2D Closer = (Here - Position).GetSafeNormal() * Enemy->GetMoveSpeed() * Time;
				const float Reach = Enemy == Boss ? Boss->GetBossRadius() + 60.f : 110.f;
				const float Gap = FVector2D::Distance(At, Position + (Enemy == Boss ? FVector2D::ZeroVector : Closer)) - Reach;
				Cost += Soon * (Gap < 0.f ? 400.f : 60.f * FMath::Exp(-Gap / 130.f));
			}
			if (Boss)
			{
				for (const FBossBolt& Bolt : Boss->GetBolts())
				{
					const float Gap = FVector2D::Distance(At, Bolt.Position + Bolt.Velocity * Time) - 60.f;
					Cost += Soon * (Gap < 0.f ? 400.f : 25.f * FMath::Exp(-Gap / 50.f));
				}
				const FBossBeam& Beam = Boss->GetBeam();
				if (Beam.State != FBossBeam::EState::Off)
				{
					const float Turn = FMath::DegreesToRadians(Beam.Angle + (Beam.State == FBossBeam::EState::Firing ? Beam.SweepRate * Time : 0.f));
					const FVector2D Along(FMath::Cos(Turn), FMath::Sin(Turn));
					const FVector2D To = At - FVector2D(Boss->GetActorLocation());
					const float Side = FVector2D::CrossProduct(Along, To);
					const float Gap = FVector2D::DotProduct(Along, To) > -50.f ? FMath::Abs(Side) - 100.f : 1000.f;
					Cost += Soon * (Gap < 0.f ? 400.f : 80.f * FMath::Exp(-Gap / 150.f));
				}
			}
			// Keep off the walls and corners.
			const float Wall = FMath::Min(FMath::Min(At.X - Bounds.Min.X, Bounds.Max.X - At.X), FMath::Min(At.Y - Bounds.Min.Y, Bounds.Max.Y - At.Y));
			Cost += Soon * 8.f * FMath::Exp(-Wall / 120.f);
			for (TActorIterator<APickup> It(GameMode.GetWorld()); It; ++It)
			{
				Cost -= Soon * 14.f * FMath::Exp(-FVector2D::Distance(At, FVector2D(It->GetActorLocation())) / 300.f);
			}
		}
		if (Cost < BestCost - 0.01f)
		{
			BestCost = Cost;
			Best = Way;
		}
	}
	LightWalker->SetMoveInput(FVector2D(Best.Y, Best.X));
}

#endif
