#include "Shield/ShieldGameMode.h"

#include "Arena/Arena.h"
#include "Core/LWFeel.h"
#include "Core/LWMusicSubsystem.h"
#include "Core/LWSound.h"
#include "Core/LightWalkerGameInstance.h"
#include "Effects/LWParticles.h"
#include "Enemies/Shaytaan.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "LightWalker.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Pickups/Pickup.h"
#include "Player/LightWalkerPlayerController.h"
#include "Shield/ShieldField.h"
#include "Shield/ShieldPawn.h"
#include "TimerManager.h"
#include "UI/ShieldHUD.h"

namespace
{
	constexpr float PlaneHeight = 50.f;
}

AShieldGameMode::AShieldGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AShieldPawn::StaticClass();
	PlayerControllerClass = ALightWalkerPlayerController::StaticClass();
	HUDClass = AShieldHUD::StaticClass();
}

AShieldPawn* AShieldGameMode::GetShieldPawn() const
{
	return Cast<AShieldPawn>(GetLightWalker());
}

int32 AShieldGameMode::GetWallCount() const
{
	return AShieldField::GetWallCount();
}

float AShieldGameMode::GetWallLeft() const
{
	return Field ? Field->GetWallLeft() : 0.f;
}

bool AShieldGameMode::IsGreatEye() const
{
	return Field && Field->HasGreatEye();
}

bool AShieldGameMode::IsBeamHeld() const
{
	return Field && Field->HasHeldBeam();
}

FString AShieldGameMode::GetWallName() const
{
	static const TCHAR* const Names[] = { TEXT("THE VEIL"), TEXT("THE KEEP"), TEXT("HORUS") };
	return Names[FMath::Clamp(Wall, 0, static_cast<int32>(UE_ARRAY_COUNT(Names)) - 1)];
}

void AShieldGameMode::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer && !NewPlayer->IsPendingKillPending())
	{
		RestartPlayerAtTransform(NewPlayer, FTransform(FVector(0.f, 0.f, PlaneHeight)));
	}
}

FVector AShieldGameMode::GetStart() const
{
	return Arena->GetCentre() + FVector(-Arena->GetHalfExtent().X + LineFromBottom, 0.f, PlaneHeight);
}

void AShieldGameMode::StartPlay()
{
	UWorld* World = GetWorld();
	Arena = World->SpawnActor<AArena>(AArena::StaticClass(), FTransform::Identity);
	if (Arena)
	{
		// The arena in other colours, open to the dark at the bottom.
		Arena->SetLook(FLinearColor(0.6f, 0.15f, 1.f), FLinearColor(1.f, 0.5f, 0.12f), FLinearColor(0.5f, 0.15f, 0.8f), true);
	}
	Field = World->SpawnActor<AShieldField>(AShieldField::StaticClass(), FTransform::Identity);
	Super::StartPlay();

	AShieldPawn* LightWalker = GetShieldPawn();
	ULightWalkerGameInstance* Run = GetRun();
	APlayerController* Controller = World->GetFirstPlayerController();
	if (!Arena || !Field || !LightWalker || !Run || !Controller)
	{
		UE_LOG(LogLightWalker, Error, TEXT("The Shield couldn't start."));
		return;
	}

	// Opened on its own (for testing), it begins a run and leads to wave 11.
	if (Run->GetLives() <= 0)
	{
		Run->StartRun(StartingLives);
		Run->SetArenaStart(EArenaStart::Continue, 10);
	}
	Run->bIntroSeen = true;

	Controller->SetViewTarget(Arena);
	LightWalker->SetField(Field);
	LightWalker->SetPlayBounds(Arena->GetPlayBounds(110.f));
	LightWalker->GiveCarriedShield();
	LightWalker->OnPickup = [this](EPickupType Type)
	{
		if (Type == EPickupType::SpreadShot)
		{
			Field->SplitBeams();
		}
	};

	Field->Setup(LightWalker, Arena->GetPlayBounds(0.f), PlaneHeight);
	Field->OnBlockHit = [this](const FWallBlock& Block, bool bBroken) { HandleBlock(Block, bBroken); };
	Field->OnDeflect = [this]()
	{
		ULWSoundSubsystem::Play(this, ELWSound::Deflect);
		ULWFeelSubsystem::Play(this, ELWFeel::Pickup);
	};
	Field->OnBeamsLost = [this]() { LoseLife(true); };
	Field->OnSaveBeam = [this]() { return BlockWithShield(GetLightWalker()); };
	Field->OnBoltBlocked = [this](const FVector& Where)
	{
		if (AShieldPawn* Pawn = GetShieldPawn())
		{
			Pawn->StrainShield();
		}
		ULWSoundSubsystem::Play(this, ELWSound::ShieldBlock);
		ULWFeelSubsystem::Play(this, ELWFeel::DajjalHit);
		LWFX::ShaytaanDeath(GetWorld(), Where);
	};
	Field->OnBoltHitPlayer = [this]() { HandlePlayerTouched(GetLightWalker()); };
	Field->OnWallCleared = [this]() { HandleWallCleared(); };

	FadeIn();
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		// Strings, melody and the 808: the drums wait for the arena.
		Music->EnsureCombat();
		Music->SetCombatWave(4);
	}

#if !UE_BUILD_SHIPPING
	// Dev only: -LWStartWall=N skips to a wall.
	int32 StartWall = 1;
	if (FParse::Value(FCommandLine::Get(), TEXT("LWStartWall="), StartWall))
	{
		Wall = FMath::Clamp(StartWall - 1, 0, GetWallCount() - 1);
	}
#endif
	BeginWall();
}

void AShieldGameMode::SetPhase(EShieldPhase NewPhase, float Duration, void (AShieldGameMode::*Next)())
{
	Phase = NewPhase;
	PhaseStart = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().ClearTimer(PhaseTimer);
	Field->SetRunning(NewPhase == EShieldPhase::Playing);
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		It->SetActive(NewPhase == EShieldPhase::Playing);
	}
	if (Next)
	{
		GetWorldTimerManager().SetTimer(PhaseTimer, this, Next, FMath::Max(Duration, 0.01f), false);
	}
}

void AShieldGameMode::BeginWall()
{
	ClearStrays(true);
	Field->BuildWall(Wall);
	if (AShieldPawn* LightWalker = GetShieldPawn())
	{
		LightWalker->PlaceAt(GetStart(), 0.f);
		LightWalker->SetHeld(false);
	}
	Field->HoldBeam();
	SetPhase(EShieldPhase::WallIntro, IntroDuration, &AShieldGameMode::BeginPlaying);
}

void AShieldGameMode::BeginPlaying()
{
	ULWSoundSubsystem::Play(this, ELWSound::WaveStart);
	SetPhase(EShieldPhase::Playing);
}

void AShieldGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AShieldPawn* LightWalker = GetShieldPawn();
	if (Phase != EShieldPhase::Playing || !LightWalker || LightWalker->IsDowned())
	{
		return;
	}

	// A freed shaytaan that runs onto the shield is knocked out by it, but the blow narrows the shield.
	const FVector2D Facing = LightWalker->GetShieldFacing();
	const FVector2D Side(-Facing.Y, Facing.X);
	const FVector2D Centre = LightWalker->GetShieldCentre();
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		const FVector2D From = FVector2D(It->GetActorLocation()) - Centre;
		const float Along = FVector2D::DotProduct(From, Side);
		const float Out = FVector2D::DotProduct(From, Facing);
		if (!It->IsDead() && FMath::Abs(Along) < LightWalker->GetShieldHalfWidth() + 20.f && Out > -70.f && Out < 60.f)
		{
			It->Kill();
			LightWalker->StrainShield();
			break;
		}
	}
}

void AShieldGameMode::HandleBlock(const FWallBlock& Block, bool bBroken)
{
	if (!bBroken)
	{
		ULWSoundSubsystem::Play(this, ELWSound::WallHit);
		ULWFeelSubsystem::Play(this, ELWFeel::DajjalHit);
		return;
	}

	ULWSoundSubsystem::Play(this, ELWSound::WallBreak);
	ULWFeelSubsystem::Play(this, ELWFeel::Kill);
	switch (Block.Kind)
	{
	case EWallBlock::Hard:
	case EWallBlock::Eye:
		GetRun()->AddScore(PointsPerHit * 2);
		break;
	case EWallBlock::Stone:
		GetRun()->AddScore(PointsPerHit * 3);
		break;
	case EWallBlock::GreatEye:
		GetRun()->AddScore(PointsForEye);
		ULWSoundSubsystem::Play(this, ELWSound::EyeBreak);
		ULWFeelSubsystem::Play(this, ELWFeel::DajjalKill);
		break;
	default:
		GetRun()->AddScore(PointsPerHit);
		break;
	}

	if (Block.Kind == EWallBlock::Vessel || Block.Kind == EWallBlock::Book)
	{
		DropPickup(Block.Centre, Block.Kind == EWallBlock::Book);
	}
	else if (Block.Kind == EWallBlock::Cage)
	{
		if (AShaytaan* Shaytaan = GetWorld()->SpawnActor<AShaytaan>(AShaytaan::StaticClass(), FVector(Block.Centre.X, Block.Centre.Y, PlaneHeight), FRotator::ZeroRotator))
		{
			Shaytaan->InitEnemy(GetLightWalker(), Arena->GetPlayBounds(50.f), ShaytaanSpeed);
			Shaytaan->SetActive(true);
		}
	}
}

void AShieldGameMode::DropPickup(const FVector2D& Where, bool bBook)
{
	if (APickup* Pickup = SpawnPickup(bBook ? EPickupType::Book : RandomPowerUp(), FVector(Where.X, Where.Y, PlaneHeight)))
	{
		Pickup->SetDrift(FVector(-PickupFallSpeed, 0.f, 0.f), 165.f);
	}
}

void AShieldGameMode::ClearStrays(bool bPickups)
{
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	if (bPickups)
	{
		for (TActorIterator<APickup> It(GetWorld()); It; ++It)
		{
			It->Destroy();
		}
	}
}

void AShieldGameMode::HandleWallCleared()
{
	GetRun()->AddScore(PointsPerWall);
	UE_LOG(LogLightWalker, Log, TEXT("[Shield] Wall %d cleared, %d lives, score %d, %.1f s"), Wall + 1, GetRun()->GetLives(), GetRun()->GetScore(), GetWorld()->GetTimeSeconds());

	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		LWFX::ShaytaanDeath(GetWorld(), It->GetActorLocation());
	}
	ClearStrays(false);
	Field->ClearBolts();

	// Whatever is still falling is caught.
	for (TActorIterator<APickup> It(GetWorld()); It; ++It)
	{
		if (It->GetType() == EPickupType::Book)
		{
			GetShieldPawn()->CollectPickup(EPickupType::Book);
		}
		It->Destroy();
	}

	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->PlaySting(ELWSting::WaveCleared);
	}
	ULWFeelSubsystem::Play(this, ELWFeel::WaveCleared);
	if (AShieldPawn* LightWalker = GetShieldPawn())
	{
		LightWalker->SetHeld(true);
		GetRun()->bCarriedShield = LightWalker->HasShield();
	}
	Field->ClearBeams();

	if (Wall + 1 >= GetWallCount())
	{
		ULWSoundSubsystem::Play(this, ELWSound::Gate);
		SetPhase(EShieldPhase::Finished, FinishDuration, &AShieldGameMode::LeaveForArena);
	}
	else
	{
		SetPhase(EShieldPhase::WallCleared, ClearedDuration, &AShieldGameMode::NextWall);
	}
}

void AShieldGameMode::NextWall()
{
	++Wall;
	BeginWall();
}

void AShieldGameMode::HandleEnemyKilled(AEnemyBase* Enemy)
{
	GetRun()->AddScore(PointsPerShaytaan);
}

void AShieldGameMode::HandlePlayerTouched(ALightWalkerPawn* LightWalker)
{
	if (Phase != EShieldPhase::Playing || !LightWalker || !LightWalker->IsVulnerable())
	{
		return;
	}
	if (BlockWithShield(LightWalker))
	{
		return;
	}
	LoseLife(false);
}

void AShieldGameMode::LoseLife(bool bFromBeam)
{
	bBeamLost = bFromBeam;
	ULWSoundSubsystem::Play(this, bFromBeam ? ELWSound::BeamLost : ELWSound::LifeLost);
	ULWFeelSubsystem::Play(this, ELWFeel::LifeLost);

	if (AShieldPawn* LightWalker = GetShieldPawn())
	{
		if (!bFromBeam)
		{
			LWFX::LifeLost(GetWorld(), LightWalker->GetActorLocation());
			LightWalker->SetDowned(true);
		}
		LightWalker->SetHeld(true);
		LightWalker->ClearPowerUps();
	}
	Field->ClearBeams();
	Field->ClearBolts();

	const int32 Lives = GetRun()->LoseLife();
	UE_LOG(LogLightWalker, Log, TEXT("[Shield] Life lost (%s), wall %d, %d blocks left, %d lives"), bFromBeam ? TEXT("beam") : TEXT("hit"), Wall + 1, Field->GetBlocksLeft(), Lives);
	if (Lives <= 0)
	{
		SetPhase(EShieldPhase::GameOver);
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
	SetPhase(EShieldPhase::Down, RespawnDelay, &AShieldGameMode::Respawn);
}

void AShieldGameMode::Respawn()
{
	ClearStrays(true);
	if (AShieldPawn* LightWalker = GetShieldPawn())
	{
		LightWalker->PlaceAt(GetStart(), 1.f);
		LightWalker->SetHeld(false);
		LWFX::Respawn(GetWorld(), LightWalker->GetActorLocation());
	}
	ULWSoundSubsystem::Play(this, ELWSound::Respawn);
	Field->HoldBeam();
	SetPhase(EShieldPhase::Playing);
}

void AShieldGameMode::LeaveForArena()
{
	SetPhase(EShieldPhase::Leaving);
	TravelTo(LWMaps::Arena, LWMaps::ArenaMode);
}

void AShieldGameMode::RequestRestart()
{
	if (IsRunOver() && GetWorld()->GetTimeSeconds() - PhaseStart >= RestartLockout)
	{
		ShowRunEnd();
	}
}

void AShieldGameMode::RestartRun()
{
	GetRun()->SetArenaStart(EArenaStart::NewRun);
	LeaveForArena();
}

void AShieldGameMode::ReturnToMenu()
{
	GetRun()->SetArenaStart(EArenaStart::Menu);
	LeaveForArena();
}

void AShieldGameMode::AddCameraTrauma(float Amount)
{
	if (Arena)
	{
		Arena->AddTrauma(Amount);
	}
}
