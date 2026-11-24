#include "Path/PathGameMode.h"

#include "Core/LWFeel.h"
#include "Core/LWMusicSubsystem.h"
#include "Core/LWSound.h"
#include "Core/LightWalkerGameInstance.h"
#include "Effects/LWParticles.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LightWalker.h"
#include "Path/PathActors.h"
#include "Path/PathCourse.h"
#include "Path/PathPawn.h"
#include "Pickups/Pickup.h"
#include "Player/Laser.h"
#include "Player/LightWalkerPlayerController.h"
#include "TimerManager.h"
#include "UI/PathHUD.h"

APathGameMode::APathGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = APathPawn::StaticClass();
	PlayerControllerClass = ALightWalkerPlayerController::StaticClass();
	HUDClass = APathHUD::StaticClass();
}

APathPawn* APathGameMode::GetPathPawn() const
{
	return Cast<APathPawn>(GetLightWalker());
}

void APathGameMode::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer && !NewPlayer->IsPendingKillPending())
	{
		RestartPlayerAtTransform(NewPlayer, FTransform::Identity);
	}
}

void APathGameMode::StartPlay()
{
	Course = GetWorld()->SpawnActor<APathCourse>(APathCourse::StaticClass(), FTransform::Identity);
	Super::StartPlay();

	APathPawn* LightWalker = GetPathPawn();
	ULightWalkerGameInstance* Run = GetRun();
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	if (!Course || !LightWalker || !Run || !Controller)
	{
		UE_LOG(LogLightWalker, Error, TEXT("The Path couldn't start."));
		return;
	}

	// Opened on its own (for testing), it begins a run and leads to wave 6.
	if (Run->GetLives() <= 0)
	{
		Run->StartRun(StartingLives);
		Run->SetArenaStart(EArenaStart::Continue, 5);
	}
	Run->bIntroSeen = true;

	Controller->SetViewTarget(Course);
	Controller->bShowMouseCursor = true;
	LightWalker->SetCourse(Course);
	LightWalker->GiveCarriedShield();
	LightWalker->PlaceAt(Course->GetCheckpoints()[0], 0.f);
	LightWalker->SetHeld(true);
	Course->LightCheckpoint(0);
	Course->Follow(LightWalker->GetCameraFocus(), 1.f, LightWalker->GetLightReach(), true);
	SpawnField();
	FadeIn();

	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		// Just the strings and melody here: the drums wait for the arena.
		Music->EnsureCombat();
		Music->SetCombatWave(1);
	}
	SetPhase(EPathPhase::Intro, IntroDuration, &APathGameMode::BeginRunning);
}

void APathGameMode::SpawnField()
{
	APathPawn* LightWalker = GetPathPawn();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FVector2D& Spot : Course->GetOrbSpots())
	{
		if (APathOrb* Orb = GetWorld()->SpawnActor<APathOrb>(APathOrb::StaticClass(), FVector(0.f, Spot.X, Spot.Y), FRotator::ZeroRotator, Params))
		{
			Orb->InitOrb(LightWalker);
			Orb->OnTaken = [this](APathOrb*, bool bFirst)
			{
				Light = FMath::Min(1.f, Light + OrbLight);
				if (bFirst)
				{
					GetRun()->AddScore(PointsPerOrb);
				}
			};
			Orbs.Add(Orb);
		}
	}

	for (const FPathSpawn& Spawn : Course->GetSpawns())
	{
		const FVector Location(0.f, Spawn.Y, Spawn.Z + 50.f);
		AEnemyBase* Enemy = nullptr;
		if (Spawn.Kind == EPathSpawn::Shaytaan)
		{
			APathShaytaan* Shaytaan = GetWorld()->SpawnActor<APathShaytaan>(APathShaytaan::StaticClass(), Location, FRotator::ZeroRotator, Params);
			if (Shaytaan)
			{
				Shaytaan->SetPatrol(Spawn.PatrolLeft, Spawn.PatrolRight);
			}
			Enemy = Shaytaan;
		}
		else if (Spawn.Kind == EPathSpawn::Dajjal)
		{
			Enemy = GetWorld()->SpawnActor<APathDajjal>(APathDajjal::StaticClass(), Location, FRotator::ZeroRotator, Params);
		}
		else if (APickup* Book = GetWorld()->SpawnActor<APickup>(APickup::StaticClass(), Location, FRotator::ZeroRotator, Params))
		{
			Book->InitPickup(EPickupType::Book, LightWalker);
			Book->SetPersistent();
		}
		if (Enemy)
		{
			Enemy->InitEnemy(LightWalker, FBox2D(ForceInit), 0.f);
		}
	}
}

void APathGameMode::SetEnemiesActive(bool bActive)
{
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		It->SetActive(bActive);
	}
}

void APathGameMode::SetPhase(EPathPhase NewPhase, float Duration, void (APathGameMode::*Next)())
{
	Phase = NewPhase;
	PhaseStart = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().ClearTimer(PhaseTimer);
	if (Next)
	{
		GetWorldTimerManager().SetTimer(PhaseTimer, this, Next, FMath::Max(Duration, 0.01f), false);
	}
}

float APathGameMode::GetPhaseAge() const
{
	return GetWorld()->GetTimeSeconds() - PhaseStart;
}

void APathGameMode::BeginRunning()
{
	if (APathPawn* LightWalker = GetPathPawn())
	{
		LightWalker->SetHeld(false);
	}
	SetEnemiesActive(true);
	ULWSoundSubsystem::Play(this, ELWSound::WaveStart);
	SetPhase(EPathPhase::Running);
}

void APathGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APathPawn* LightWalker = GetPathPawn();
	if (!Course || !LightWalker)
	{
		return;
	}

	if (Phase == EPathPhase::Running)
	{
		UpdateLight(DeltaSeconds);
		UpdateCheckpoints();

		const EPathHazard Hazard = LightWalker->TakeHazard();
		if (Phase == EPathPhase::Running && Hazard != EPathHazard::None)
		{
			// The shield saves one fall or one stumble onto spikes, like any other hit.
			if (LightWalker->TryBlockHit())
			{
				ULWSoundSubsystem::Play(this, ELWSound::ShieldBlock);
				ULWFeelSubsystem::Play(this, ELWFeel::ShieldBlock);
				LWFX::ShieldBlock(GetWorld(), LightWalker->GetActorLocation());
				if (Hazard == EPathHazard::Fell)
				{
					LightWalker->PlaceAt(Course->GetCheckpoints()[Checkpoint], RespawnGrace);
				}
			}
			else
			{
				LoseLife(LightWalker->GetActorLocation());
			}
		}
		if (Phase == EPathPhase::Running && LightWalker->GetActorLocation().Y >= Course->GetGateY() && !LightWalker->IsDowned())
		{
			Finish();
		}
	}

	LightWalker->SetLightLevel(Light);
	Course->SetLight(Light);
	Course->Follow(LightWalker->GetCameraFocus(), LightWalker->GetFacing(), LightWalker->IsDowned() ? 120.f : LightWalker->GetLightReach());
}

void APathGameMode::UpdateLight(float DeltaSeconds)
{
	const float Lasting = LightDuration * GetRun()->GetUpgrades().LightRadius;
	Light = FMath::Max(0.f, Light - DeltaSeconds / FMath::Max(Lasting, 1.f));

	LowestLight = FMath::Min(LowestLight, Light);

	WarningTimer -= DeltaSeconds;
	if (Light < LowLight && WarningTimer <= 0.f)
	{
		// Quicker as it gets closer to going out.
		WarningTimer = FMath::Lerp(0.45f, 1.1f, Light / LowLight);
		ULWSoundSubsystem::Play(this, ELWSound::LightLow);
	}
	if (Light <= 0.f)
	{
		bWentDark = true;
		if (APathPawn* LightWalker = GetPathPawn())
		{
			LoseLife(LightWalker->GetActorLocation());
		}
	}
}

void APathGameMode::UpdateCheckpoints()
{
	const APathPawn* LightWalker = GetPathPawn();
	const TArray<FVector2D>& Spots = Course->GetCheckpoints();
	while (Spots.IsValidIndex(Checkpoint + 1) && LightWalker->IsGrounded() && LightWalker->GetActorLocation().Y >= Spots[Checkpoint + 1].X)
	{
		++Checkpoint;
		Light = 1.f;
		Course->LightCheckpoint(Checkpoint);
		ULWSoundSubsystem::Play(this, ELWSound::Checkpoint);
		ULWFeelSubsystem::Play(this, ELWFeel::Pickup);
		LWFX::Respawn(GetWorld(), FVector(0.f, Spots[Checkpoint].X, Spots[Checkpoint].Y + 60.f));
	}
}

void APathGameMode::HandleEnemyKilled(AEnemyBase* Enemy)
{
	GetRun()->AddScore(Enemy && Enemy->IsA<ADajjal>() ? PointsPerDajjal : PointsPerShaytaan);
}

void APathGameMode::HandlePlayerTouched(ALightWalkerPawn* LightWalker)
{
	if (Phase != EPathPhase::Running || !LightWalker || !LightWalker->IsVulnerable())
	{
		return;
	}
	if (LightWalker->TryBlockHit())
	{
		ULWSoundSubsystem::Play(this, ELWSound::ShieldBlock);
		ULWFeelSubsystem::Play(this, ELWFeel::ShieldBlock);
		LWFX::ShieldBlock(GetWorld(), LightWalker->GetActorLocation());
		return;
	}
	LoseLife(LightWalker->GetActorLocation());
}

void APathGameMode::LoseLife(const FVector& Location)
{
	ULWSoundSubsystem::Play(this, ELWSound::LifeLost);
	ULWFeelSubsystem::Play(this, ELWFeel::LifeLost);
	LWFX::LifeLost(GetWorld(), Location);

	if (APathPawn* LightWalker = GetPathPawn())
	{
		LightWalker->SetDowned(true);
		LightWalker->ClearPowerUps();
	}

	const int32 Lives = GetRun()->LoseLife();
	UE_LOG(LogLightWalker, Log, TEXT("[Path] Life lost at Y %.0f Z %.0f, light %.2f, %d left"), Location.Y, Location.Z, Light, Lives);
	if (Lives <= 0)
	{
		SetEnemiesActive(false);
		SetPhase(EPathPhase::GameOver);
		EndRun(2.6f);
		if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
		{
			Music->PlaySting(ELWSting::GameOver);
		}
		return;
	}
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->DuckCombat(RespawnDelay);
	}
	SetPhase(EPathPhase::Down, RespawnDelay, &APathGameMode::Respawn);
}

void APathGameMode::Respawn()
{
	APathPawn* LightWalker = GetPathPawn();
	if (!LightWalker)
	{
		return;
	}

	const FVector2D Spot = Course->GetCheckpoints()[Checkpoint];
	Light = 1.f;
	bWentDark = false;
	Course->ResetCrumbling();
	for (APathOrb* Orb : Orbs)
	{
		if (Orb && Orb->IsTaken() && Orb->GetActorLocation().Y > Spot.X)
		{
			Orb->Restore();
		}
	}
	for (TActorIterator<ALaser> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	LightWalker->PlaceAt(Spot, RespawnGrace);
	ULWSoundSubsystem::Play(this, ELWSound::Respawn);
	LWFX::Respawn(GetWorld(), LightWalker->GetActorLocation());
	Course->Follow(LightWalker->GetCameraFocus(), 1.f, LightWalker->GetLightReach(), true);
	SetPhase(EPathPhase::Running);
}

void APathGameMode::Finish()
{
	FinishBonus = FinishPoints + FMath::RoundToInt(FinishPoints * Light / 50.f) * 50;
	GetRun()->AddScore(FinishBonus);
	UE_LOG(LogLightWalker, Log, TEXT("[Path] Finished with %d lives, light %.2f (lowest %.2f), score %d, %.1f s"), GetRun()->GetLives(), Light, LowestLight, GetRun()->GetScore(), GetWorld()->GetTimeSeconds());
	Light = 1.f;
	SetEnemiesActive(false);
	if (APathPawn* LightWalker = GetPathPawn())
	{
		LightWalker->SetHeld(true);
		LWFX::PickupCollect(GetWorld(), LightWalker->GetActorLocation(), FLinearColor(1.f, 0.85f, 0.45f), true);
		GetRun()->bCarriedShield = LightWalker->HasShield();
	}
	ULWSoundSubsystem::Play(this, ELWSound::Gate);
	ULWFeelSubsystem::Play(this, ELWFeel::WaveCleared);
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->PlaySting(ELWSting::WaveCleared);
	}
	SetPhase(EPathPhase::Finished, FinishDuration, &APathGameMode::LeaveForArena);
}

void APathGameMode::LeaveForArena()
{
	SetPhase(EPathPhase::Leaving);
	TravelTo(LWMaps::Arena, LWMaps::ArenaMode);
}

void APathGameMode::RequestRestart()
{
	if (IsRunOver() && GetPhaseAge() >= RestartLockout)
	{
		ShowRunEnd();
	}
}

void APathGameMode::RestartRun()
{
	GetRun()->SetArenaStart(EArenaStart::NewRun);
	LeaveForArena();
}

void APathGameMode::ReturnToMenu()
{
	GetRun()->SetArenaStart(EArenaStart::Menu);
	LeaveForArena();
}

void APathGameMode::AddCameraTrauma(float Amount)
{
	if (Course)
	{
		Course->AddTrauma(Amount);
	}
}

bool APathGameMode::BlocksShot(const FVector& Location) const
{
	return Course && Course->IsSolidAt(Location.Y, Location.Z);
}

float APathGameMode::GetProgress() const
{
	const APathPawn* LightWalker = GetPathPawn();
	const float Start = Course ? Course->GetCheckpoints()[0].X : 0.f;
	return Course && LightWalker ? FMath::Clamp((LightWalker->GetActorLocation().Y - Start) / FMath::Max(Course->GetGateY() - Start, 1.f), 0.f, 1.f) : 0.f;
}

TArray<float> APathGameMode::GetCheckpointMarks() const
{
	TArray<float> Marks;
	if (Course)
	{
		const float Start = Course->GetCheckpoints()[0].X;
		for (const FVector2D& Spot : Course->GetCheckpoints())
		{
			Marks.Add((Spot.X - Start) / FMath::Max(Course->GetGateY() - Start, 1.f));
		}
	}
	return Marks;
}
