#include "Arena/ArenaGameMode.h"

#include "Arena/Arena.h"
#include "Core/LWMusicSubsystem.h"
#include "Core/LWFeel.h"
#include "Core/LWSound.h"
#include "Effects/LWParticles.h"
#include "Core/LightWalkerGameInstance.h"
#include "Arena/ArenaAutoPlay.h"
#include "Enemies/Dajjal.h"
#include "Enemies/OneEyedLiar.h"
#include "Enemies/Shaytaan.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "LightWalker.h"
#include "Pickups/Pickup.h"
#include "Player/Laser.h"
#include "Player/LightWalkerPawn.h"
#include "Player/LightWalkerPlayerController.h"
#include "TimerManager.h"
#include "UI/ArenaHUD.h"

namespace
{
	FArenaWave MakeWave(int32 Shaytaan, int32 Dajjal, float StartSpeed, float TopSpeed)
	{
		FArenaWave Wave;
		Wave.ShaytaanCount = Shaytaan;
		Wave.DajjalCount = Dajjal;
		Wave.ShaytaanSpeed = StartSpeed;
		Wave.ShaytaanTopSpeed = TopSpeed;
		Wave.DajjalSpeed = 100.f;
		return Wave;
	}

	/**
	 * Waves 1 and 2 are the 2014 levels. From there each wave adds chasers and wanderers and starts and tops out
	 * a little faster. Waves 5 and 10 change the pace: fewer chasers, a crowd of wanderers. Wave 15 is the
	 * hardest: 60 chasers, 14 wanderers, up to 480 cm/s (Lightwalker moves at 720).
	 */
	TArray<FArenaWave> MakeDefaultWaves()
	{
		TArray<FArenaWave> Waves = { MakeWave(15, 0, 100.f, 180.f), MakeWave(30, 5, 150.f, 260.f) };
		for (int32 Number = 3; Number <= 15; ++Number)
		{
			const float Progress = float(Number - 2) / 13.f;
			const bool bWandererWave = Number == 5 || Number == 10;
			FArenaWave Wave = MakeWave(
				FMath::RoundToInt(FMath::Lerp(32.f, 60.f, Progress) * (bWandererWave ? 0.6f : 1.f)),
				FMath::RoundToInt(FMath::Lerp(6.f, 14.f, Progress) * (bWandererWave ? 1.8f : 1.f)),
				FMath::Lerp(160.f, 300.f, Progress),
				FMath::Lerp(280.f, 480.f, Progress));
			Wave.DajjalSpeed = FMath::Lerp(105.f, 170.f, Progress);
			Waves.Add(Wave);
		}
		return Waves;
	}
}

AArenaGameMode::AArenaGameMode()
{
	DefaultPawnClass = ALightWalkerPawn::StaticClass();
	PlayerControllerClass = ALightWalkerPlayerController::StaticClass();
	HUDClass = AArenaHUD::StaticClass();

	ArenaClass = AArena::StaticClass();
	ShaytaanClass = AShaytaan::StaticClass();
	DajjalClass = ADajjal::StaticClass();

	PrimaryActorTick.bCanEverTick = true;
	BossClass = AOneEyedLiar::StaticClass();
	DefaultWaves = MakeDefaultWaves();
	PickupClass = APickup::StaticClass();
}

const TArray<FArenaWave>& AArenaGameMode::GetWaves() const
{
	return (WaveSet && WaveSet->Waves.Num() > 0) ? WaveSet->Waves : DefaultWaves;
}

void AArenaGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	// The arena needs no PlayerStart: Lightwalker always begins in the centre.
	const FVector Centre = Arena ? Arena->GetCentre() : FVector::ZeroVector;
	RestartPlayerAtTransform(NewPlayer, FTransform(FRotator::ZeroRotator, Centre + FVector(0.f, 0.f, PlaneHeight)));
}

void AArenaGameMode::StartPlay()
{
	UWorld* World = GetWorld();

	for (TActorIterator<AArena> It(World); It; ++It)
	{
		Arena = *It;
		break;
	}
	if (!Arena)
	{
		Arena = World->SpawnActor<AArena>(ArenaClass ? ArenaClass.Get() : AArena::StaticClass(), FTransform::Identity);
	}

	Super::StartPlay();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* Controller = It->Get())
		{
			Controller->SetViewTarget(Arena);
		}
	}

	if (!GetRun())
	{
		UE_LOG(LogLightWalker, Error, TEXT("Game instance class isn't LightWalkerGameInstance; check GameInstanceClass in DefaultEngine.ini."));
		return;
	}

	// Travelled here from another mode: pick the run up, or start a new one.
	const EArenaStart Start = GetRun()->TakeArenaStart();
	if (Start != EArenaStart::Menu)
	{
		FadeIn();
		if (Start == EArenaStart::Continue)
		{
			ContinueRun(GetRun()->GetNextWave());
		}
		else
		{
			StartRun();
		}
		return;
	}

	// Launch into the intro and main menu. Dev runs that ask for a wave skip straight to it.
	bool bSkipMenu = false;
#if !UE_BUILD_SHIPPING
	int32 RequestedWave = 0;
	bSkipMenu = !GetRun()->bIntroSeen && FParse::Value(FCommandLine::Get(), TEXT("LWStartWave="), RequestedWave);
#endif
	if (bSkipMenu)
	{
		GetRun()->bIntroSeen = true;
		StartRun();
	}
	else
	{
		EnterMenu(true);
		if (ALightWalkerPlayerController* Controller = Cast<ALightWalkerPlayerController>(World->GetFirstPlayerController()))
		{
#if !UE_BUILD_SHIPPING
			// -LWMenu skips the intro (for screenshots of the menu).
			if (FParse::Param(FCommandLine::Get(), TEXT("LWMenu")))
			{
				Controller->ShowMainMenu();

				// -LWPage=settings or -LWPage=controls opens that page over it.
				FString Page;
				if (FParse::Value(FCommandLine::Get(), TEXT("LWPage="), Page))
				{
					Controller->OpenSettings();
					if (Page == TEXT("controls"))
					{
						Controller->OpenControls();
					}
				}
			}
			else
#endif
			if (GetRun()->bIntroSeen)
			{
				FadeIn();
				Controller->ShowMainMenu();
			}
			else
			{
				Controller->ShowIntro();
			}
			GetRun()->bIntroSeen = true;
		}
	}

}

void AArenaGameMode::AddCameraTrauma(float Amount)
{
	if (Arena)
	{
		Arena->AddTrauma(Amount);
	}
}

void AArenaGameMode::EnterMenu(bool bInstantCamera)
{
	GetWorldTimerManager().ClearTimer(PhaseTimer);
	ClearField();
	Arena->SetMood(0.f);
	Arena->SetMenuView(true, bInstantCamera);

	if (ALightWalkerPawn* LightWalker = GetLightWalker())
	{
		LightWalker->ClearPowerUps();
		LightWalker->Revive(Arena->GetCentre(), 0.f);
		LightWalker->SetFrozen(true);
	}

	SetPhase(EArenaPhase::Menu, 0.f, nullptr);
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->PlayMenu();
	}
}

void AArenaGameMode::StartFromMenu()
{
	if (Phase != EArenaPhase::Menu)
	{
		return;
	}
	Arena->SetMenuView(false);
	StartRun();
}

void AArenaGameMode::ReturnToMenu()
{
	EnterMenu(false);
}

void AArenaGameMode::StartRun()
{
	ClearField();
	Arena->SetMood(0.f);
	GetRun()->StartRun(StartingLives);
	Arena->SetMenuView(false);
	if (ALightWalkerPawn* LightWalker = GetLightWalker())
	{
		LightWalker->ClearPowerUps();
		LightWalker->SetFrozen(false);
	}
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->PlayCombat();
	}
	WaveIndex = 0;

#if !UE_BUILD_SHIPPING
	// Dev only: -LWStartWave=N skips straight to wave N.
	int32 StartWave = 1;
	if (FParse::Value(FCommandLine::Get(), TEXT("LWStartWave="), StartWave))
	{
		// One past the last wave is the boss.
		WaveIndex = FMath::Clamp(StartWave - 1, 0, GetWaveCount());
	}
#endif
	BeginWaveIntro();
}

void AArenaGameMode::ContinueRun(int32 FromWaveIndex)
{
	ClearField();
	Arena->SetMenuView(false, true);
	if (ALightWalkerPawn* LightWalker = GetLightWalker())
	{
		LightWalker->ClearPowerUps();
		LightWalker->SetFrozen(false);
		LightWalker->GiveCarriedShield();
	}
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->EnsureCombat();
	}
	WaveIndex = FromWaveIndex;
	BeginWaveIntro();
}

bool AArenaGameMode::LeadsToPath() const
{
	return PathAfterWave > 0 && WaveIndex + 1 == PathAfterWave && MapExists(LWMaps::Path);
}

bool AArenaGameMode::LeadsToShield() const
{
	return ShieldAfterWave > 0 && WaveIndex + 1 == ShieldAfterWave && MapExists(LWMaps::Shield);
}

void AArenaGameMode::SetPhase(EArenaPhase NewPhase, float Duration, void (AArenaGameMode::*Next)())
{
	Phase = NewPhase;
	PhaseStartTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().ClearTimer(PhaseTimer);
	if (Next)
	{
		GetWorldTimerManager().SetTimer(PhaseTimer, this, Next, FMath::Max(Duration, 0.01f), false);
	}
}

void AArenaGameMode::BeginWaveIntro()
{
	const TArray<FArenaWave>& Waves = GetWaves();
	if (IsBossWave())
	{
		BeginBossIntro();
		return;
	}
	if (!Waves.IsValidIndex(WaveIndex))
	{
		SetPhase(EArenaPhase::Victory, 0.f, nullptr);
		return;
	}

	if (ALightWalkerPawn* LightWalker = GetLightWalker())
	{
		LightWalker->SetPlayBounds(Arena->GetPlayBounds(LightWalker->GetContactRadius()));
		LightWalker->Revive(Arena->GetCentre(), 0.f);
	}

	bBookDroppedThisWave = false;
	SpawnWave(Waves[WaveIndex]);
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->SetCombatWave(WaveIndex + 1);
	}
	SetPhase(EArenaPhase::WaveIntro, WaveIntroDuration, &AArenaGameMode::BeginFighting);
}

void AArenaGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
	// Dev only: -LWAutoFight plays the arena and the boss by itself, to test them.
	static const bool bAutoFight = FParse::Param(FCommandLine::Get(), TEXT("LWAutoFight"));
	if (bAutoFight)
	{
		LWAutoPlay::Fight(*this, GetLightWalker());
	}
#endif
}

void AArenaGameMode::BeginBossIntro()
{
	ClearField();
	if (ALightWalkerPawn* LightWalker = GetLightWalker())
	{
		LightWalker->SetPlayBounds(Arena->GetPlayBounds(LightWalker->GetContactRadius()));
		LightWalker->Revive(Arena->GetCentre() - FVector(Arena->GetHalfExtent().X * 0.55f, 0.f, 0.f), 0.f);
	}
	bBookDroppedThisWave = false;
	BossBonus = 0;
	Arena->SetMood(1.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Boss = GetWorld()->SpawnActor<AOneEyedLiar>(BossClass, Arena->GetCentre() + FVector(520.f, 0.f, PlaneHeight), FRotator(0.f, 0.f, 0.f), Params);
	if (!Boss)
	{
		SetPhase(EArenaPhase::Victory, 0.f, nullptr);
		return;
	}
	Boss->InitEnemy(GetLightWalker(), Arena->GetPlayBounds(0.f), 0.f);
	Boss->Arrive(Arena->GetCentre(), BossIntroDuration - 0.8f);
	Boss->OnSummon = [this](int32 Shaytaan, int32 Dajjal) { SummonForBoss(Shaytaan, Dajjal); };
	Boss->OnStage = [this](int32 Stage) { HandleBossStage(Stage); };
	Boss->OnDefeated = [this]() { HandleBossDefeated(); };
	Boss->OnGone = [this]() { HandleBossGone(); };
	Boss->OnSpawn = [this](TSubclassOf<AEnemyBase> Class, const FVector& Where) -> AEnemyBase*
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AEnemyBase* Enemy = GetWorld()->SpawnActor<AEnemyBase>(Class, Where, FRotator::ZeroRotator, SpawnParams);
		if (Enemy)
		{
			Enemy->InitEnemy(GetLightWalker(), Arena->GetPlayBounds(50.f), 0.f);
			Enemy->SetActive(Phase == EArenaPhase::Fighting || Phase == EArenaPhase::PlayerDown);
			Enemies.Add(Enemy);
		}
		return Enemy;
	};
	Enemies.Add(Boss);

	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->EnsureCombat();
		Music->SetCombatWave(GetWaveCount());
		Music->DuckCombat(BossIntroDuration - 1.f);
	}
	ULWSoundSubsystem::Play(this, ELWSound::BossArrive);
	SetPhase(EArenaPhase::BossIntro, BossIntroDuration, &AArenaGameMode::BeginBossFight);
}

void AArenaGameMode::BeginBossFight()
{
	SetEnemiesActive(true);
	SetPhase(EArenaPhase::Fighting, 0.f, nullptr);
	ULWSoundSubsystem::Play(this, ELWSound::WaveStart);
	GetWorldTimerManager().SetTimer(BossPickupTimer, this, &AArenaGameMode::DropBossPickup, BossPickupInterval, true, BossPickupInterval * 0.5f);
}

void AArenaGameMode::SummonForBoss(int32 Shaytaan, int32 Dajjal)
{
	// Never more than a handful at once: the boss is the fight.
	const int32 Room = FMath::Max(0, 14 - Enemies.Num());
	const FBox2D Bounds = Arena->GetPlayBounds(60.f);
	const ALightWalkerPawn* LightWalker = GetLightWalker();
	const FVector2D Player = LightWalker ? FVector2D(LightWalker->GetActorLocation()) : FVector2D::ZeroVector;
	for (int32 Index = 0; Index < FMath::Min(Shaytaan + Dajjal, Room); ++Index)
	{
		FVector2D Point = Bounds.GetCenter();
		for (int32 Attempt = 0; Attempt < 30; ++Attempt)
		{
			Point = FVector2D(FMath::FRandRange(Bounds.Min.X, Bounds.Max.X), FMath::FRandRange(Bounds.Min.Y, Bounds.Max.Y));
			if (FVector2D::Distance(Point, Player) > 800.f)
			{
				break;
			}
		}
		const FVector Where(Point.X, Point.Y, Arena->GetCentre().Z + PlaneHeight);
		if (AEnemyBase* Enemy = Boss->OnSpawn(Index < Dajjal ? TSubclassOf<AEnemyBase>(DajjalClass) : TSubclassOf<AEnemyBase>(ShaytaanClass), Where))
		{
			Enemy->SetMoveSpeed(Index < Dajjal ? SummonSpeed * 0.6f : SummonSpeed);
			LWFX::ShaytaanDeath(GetWorld(), Where);
		}
	}
}

void AArenaGameMode::DropBossPickup()
{
	if (Phase != EArenaPhase::Fighting || !PickupClass)
	{
		return;
	}
	const FBox2D Bounds = Arena->GetPlayBounds(250.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Where(FMath::FRandRange(Bounds.Min.X, Bounds.GetCenter().X), FMath::FRandRange(Bounds.Min.Y, Bounds.Max.Y), Arena->GetCentre().Z + PlaneHeight);
	if (APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, Where, FRotator::ZeroRotator, Params))
	{
		Pickup->InitPickup(static_cast<EPickupType>(FMath::RandRange(0, static_cast<int32>(EPickupType::Shield))), GetLightWalker());
		LWFX::Respawn(GetWorld(), Where);
	}
}

void AArenaGameMode::HandleBossStage(int32 Stage)
{
	UE_LOG(LogLightWalker, Log, TEXT("[Boss] Stage %d, %d lives, %.1f s"), Stage, GetRun()->GetLives(), GetWorld()->GetTimeSeconds() - PhaseStartTime);
	ULWSoundSubsystem::Play(this, ELWSound::BossArrive);
	ULWFeelSubsystem::Play(this, ELWFeel::BossDown);
	DropBossPickup();
}

void AArenaGameMode::HandleBossDefeated()
{
	GetWorldTimerManager().ClearTimer(BossPickupTimer);
	BossBonus = PointsForBoss + PointsPerLife * GetRun()->GetLives();
	GetRun()->AddScore(BossBonus);
	UE_LOG(LogLightWalker, Log, TEXT("[Boss] Defeated with %d lives, score %d"), GetRun()->GetLives(), GetRun()->GetScore());

	// Everything it called goes with it.
	const TArray<TObjectPtr<AEnemyBase>> All = Enemies;
	for (AEnemyBase* Enemy : All)
	{
		if (Enemy && Enemy != Boss)
		{
			LWFX::ShaytaanDeath(GetWorld(), Enemy->GetActorLocation());
			Enemies.Remove(Enemy);
			Enemy->Destroy();
		}
	}
	// Lightwalker sees it fall, even if it took them down with its last blow.
	ALightWalkerPawn* LightWalker = GetLightWalker();
	if (LightWalker && LightWalker->IsDowned())
	{
		LightWalker->Revive(FindRespawnPoint(), 0.f);
	}
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->StopAll(3.f);
	}
	SetPhase(EArenaPhase::BossFalling, 0.f, nullptr);
}

void AArenaGameMode::HandleBossGone()
{
	Enemies.Remove(Boss);
	Boss = nullptr;
	Arena->SetMood(-1.f);
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->PlaySting(ELWSting::WaveCleared);
	}
	SetPhase(EArenaPhase::Victory, 0.f, nullptr);
}

void AArenaGameMode::BeginFighting()
{
	SetEnemiesActive(true);
	SetPhase(EArenaPhase::Fighting, 0.f, nullptr);
	ULWSoundSubsystem::Play(this, ELWSound::WaveStart);

#if !UE_BUILD_SHIPPING
	// Dev only: -LWAutoWin clears each wave a moment after it starts, to test what follows.
	if (FParse::Param(FCommandLine::Get(), TEXT("LWAutoWin")))
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			const TArray<TObjectPtr<AEnemyBase>> Wave = Enemies;
			for (AEnemyBase* Enemy : Wave)
			{
				if (Enemy)
				{
					Enemy->Kill();
				}
			}
		}), 1.f, false);
	}

	// Dev only: -LWDrops also lays out one of each pickup around the centre, to check how they look.
	if (FParse::Param(FCommandLine::Get(), TEXT("LWDrops")) && PickupClass)
	{
		for (int32 Index = 0; Index < static_cast<int32>(EPickupType::Count); ++Index)
		{
			const FVector Offset = FRotator(0.f, 360.f * Index / static_cast<int32>(EPickupType::Count), 0.f).RotateVector(FVector(260.f, 0.f, 0.f));
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, Arena->GetCentre() + Offset + FVector(0.f, 0.f, PlaneHeight), FRotator::ZeroRotator, Params))
			{
				Pickup->InitPickup(static_cast<EPickupType>(Index), GetLightWalker());
			}
		}
	}
#endif
}

void AArenaGameMode::SpawnWave(const FArenaWave& Wave)
{
	UWorld* World = GetWorld();
	ALightWalkerPawn* LightWalker = GetLightWalker();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto SpawnEnemies = [&](TSubclassOf<AEnemyBase> Class, int32 Count, float Speed)
	{
		if (!Class)
		{
			return;
		}
		for (int32 Index = 0; Index < Count; ++Index)
		{
			AEnemyBase* Enemy = World->SpawnActor<AEnemyBase>(Class, RandomEnemySpawn(), FRotator::ZeroRotator, Params);
			if (Enemy)
			{
				Enemy->InitEnemy(LightWalker, Arena->GetPlayBounds(50.f), Speed);
				Enemies.Add(Enemy);
			}
		}
	};

#if !UE_BUILD_SHIPPING
	// Dev only: -LWAutoRun (the hero running on the spot for screenshots) leaves the arena empty.
	if (FParse::Param(FCommandLine::Get(), TEXT("LWAutoRun")))
	{
		return;
	}
#endif
	SpawnEnemies(ShaytaanClass, Wave.ShaytaanCount, Wave.ShaytaanSpeed);
	SpawnEnemies(DajjalClass, Wave.DajjalCount, Wave.DajjalSpeed);
	ApplyChaserSpeed();
}

FVector AArenaGameMode::RandomEnemySpawn() const
{
	const FBox2D Bounds = Arena->GetPlayBounds(60.f);
	const FVector Centre = Arena->GetCentre();

	// Anywhere outside the safe zone. (The 2014 check needed both axes outside it, which left only the corners.)
	FVector2D Point = Bounds.Min;
	for (int32 Attempt = 0; Attempt < 50; ++Attempt)
	{
		Point = FVector2D(FMath::FRandRange(Bounds.Min.X, Bounds.Max.X), FMath::FRandRange(Bounds.Min.Y, Bounds.Max.Y));
		const bool bOutsideSafeZone =
			FMath::Abs(Point.X - Centre.X) > SafeZoneHalfExtent.X ||
			FMath::Abs(Point.Y - Centre.Y) > SafeZoneHalfExtent.Y;
		if (bOutsideSafeZone)
		{
			break;
		}
	}
	return FVector(Point.X, Point.Y, Centre.Z + PlaneHeight);
}

void AArenaGameMode::ApplyChaserSpeed()
{
	const TArray<FArenaWave>& Waves = GetWaves();
	if (!Waves.IsValidIndex(WaveIndex))
	{
		return;
	}

	const FArenaWave& Wave = Waves[WaveIndex];

	// 0 with the whole wave alive, 1 when only one enemy is left.
	const int32 Total = Wave.GetEnemyCount();
	const float Thinned = Total > 1 ? FMath::Clamp(float(Total - Enemies.Num()) / float(Total - 1), 0.f, 1.f) : 1.f;
	const float Speed = FMath::Lerp(Wave.ShaytaanSpeed, FMath::Max(Wave.ShaytaanTopSpeed, Wave.ShaytaanSpeed),
		FMath::Pow(Thinned, Wave.SpeedRampCurve));

	for (AEnemyBase* Enemy : Enemies)
	{
		if (Enemy && Enemy->IsA<AShaytaan>())
		{
			Enemy->SetMoveSpeed(Speed);
		}
	}
}

void AArenaGameMode::HandleEnemyKilled(AEnemyBase* Enemy)
{
	if (Enemies.Remove(Enemy) == 0)
	{
		return;
	}

	const bool bDajjal = Enemy && Enemy->IsA<ADajjal>();
	GetRun()->AddScore(bDajjal ? PointsPerDajjal : PointsPerKill);
	ApplyChaserSpeed();
	if (Enemy)
	{
		TryDropPickup(Enemy->GetActorLocation(), bDajjal);
	}

	if (Boss)
	{
		return;  // the fight ends with the boss, not with what it summons
	}
	const bool bWaveRunning = Phase == EArenaPhase::WaveIntro || Phase == EArenaPhase::Fighting || Phase == EArenaPhase::PlayerDown;
	if (Enemies.Num() == 0 && bWaveRunning)
	{
		FinishWave();
	}
}

void AArenaGameMode::HandlePlayerTouched(ALightWalkerPawn* LightWalker)
{
	if (Phase != EArenaPhase::Fighting || !LightWalker || !LightWalker->IsVulnerable())
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

	ULWSoundSubsystem::Play(this, ELWSound::LifeLost);
	ULWFeelSubsystem::Play(this, ELWFeel::LifeLost);
	LWFX::LifeLost(GetWorld(), LightWalker->GetActorLocation());

	LightWalker->SetDowned(true);
	LightWalker->ClearPowerUps();
	if (Boss)
	{
		Boss->ClearAttacks();
	}

	if (GetRun()->LoseLife() <= 0)
	{
		SetEnemiesActive(false);
		SetPhase(EArenaPhase::GameOver, 0.f, nullptr);
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
	SetPhase(EArenaPhase::PlayerDown, RespawnDelay, &AArenaGameMode::RespawnLightWalker);
}

void AArenaGameMode::RespawnLightWalker()
{
	if (ALightWalkerPawn* LightWalker = GetLightWalker())
	{
		LightWalker->Revive(FindRespawnPoint(), Boss ? RespawnGrace * 2.f : RespawnGrace);
		ULWSoundSubsystem::Play(this, ELWSound::Respawn);
		LWFX::Respawn(GetWorld(), LightWalker->GetActorLocation());
	}
	SetPhase(EArenaPhase::Fighting, 0.f, nullptr);
}

FVector AArenaGameMode::FindRespawnPoint() const
{
	const FBox2D Bounds = Arena->GetPlayBounds(60.f);
	const FVector Centre = Arena->GetCentre();

	// Try random spots and keep the one furthest from any enemy, stopping early once one is clear enough.
	// (In 2014, if no spot was clear Lightwalker reappeared wherever they had died.)
	FVector2D Best(Centre.X, Centre.Y);
	float BestClearance = -1.f;

	for (int32 Attempt = 0; Attempt < RespawnAttempts; ++Attempt)
	{
		const FVector2D Point(FMath::FRandRange(Bounds.Min.X, Bounds.Max.X), FMath::FRandRange(Bounds.Min.Y, Bounds.Max.Y));

		float Clearance = TNumericLimits<float>::Max();
		for (const AEnemyBase* Enemy : Enemies)
		{
			if (Enemy)
			{
				const FVector Location = Enemy->GetActorLocation();
				const float Size = Enemy == Boss ? Boss->GetBossRadius() * 1.5f : 0.f;
				Clearance = FMath::Min(Clearance, FVector2D::Distance(Point, FVector2D(Location.X, Location.Y)) - Size);
			}
		}

		if (Clearance > BestClearance)
		{
			Best = Point;
			BestClearance = Clearance;
		}
		if (BestClearance >= RespawnClearance)
		{
			break;
		}
	}

	return FVector(Best.X, Best.Y, Centre.Z + PlaneHeight);
}

void AArenaGameMode::FinishWave()
{
	if (ALightWalkerPawn* LightWalker = GetLightWalker())
	{
		LightWalker->SetDowned(true);
	}
	ClearField();
	if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
	{
		Music->PlaySting(ELWSting::WaveCleared);
		ULWFeelSubsystem::Play(this, ELWFeel::WaveCleared);
	}
	SetPhase(EArenaPhase::WaveCleared, WaveClearedDuration, &AArenaGameMode::AdvanceWave);
}

void AArenaGameMode::AdvanceWave()
{
	if (LeadsToPath() || LeadsToShield())
	{
		if (ALightWalkerPawn* LightWalker = GetLightWalker())
		{
			GetRun()->bCarriedShield = LightWalker->HasShield();
		}
		SetPhase(EArenaPhase::Travelling, 0.f, nullptr);
		GetRun()->SetArenaStart(EArenaStart::Continue, WaveIndex + 1);
		if (LeadsToPath())
		{
			TravelTo(LWMaps::Path, LWMaps::PathMode);
		}
		else
		{
			TravelTo(LWMaps::Shield, LWMaps::ShieldMode);
		}
		return;
	}

	++WaveIndex;
	if (GetWaves().IsValidIndex(WaveIndex) || IsBossWave())
	{
		BeginWaveIntro();
	}
	else
	{
		SetPhase(EArenaPhase::Victory, 0.f, nullptr);
		if (ULWMusicSubsystem* Music = ULWMusicSubsystem::Get(this))
		{
			Music->StopAll(3.f);
		}
	}
}

void AArenaGameMode::RequestRestart()
{
	if (IsRunOver() && GetWorld()->GetTimeSeconds() - PhaseStartTime >= RestartLockout)
	{
		StartRun();
	}
}

void AArenaGameMode::SetEnemiesActive(bool bActive)
{
	for (AEnemyBase* Enemy : Enemies)
	{
		if (Enemy)
		{
			Enemy->SetActive(bActive);
		}
	}
}

void AArenaGameMode::ClearField()
{
	for (AEnemyBase* Enemy : Enemies)
	{
		if (Enemy)
		{
			Enemy->Destroy();
		}
	}
	Enemies.Reset();
	Boss = nullptr;
	GetWorldTimerManager().ClearTimer(BossPickupTimer);

	for (TActorIterator<ALaser> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	for (TActorIterator<APickup> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
}

void AArenaGameMode::TryDropPickup(const FVector& Location, bool bFromDajjal)
{
	if (!PickupClass || Phase != EArenaPhase::Fighting)
	{
		return;
	}

	float DropChance = bFromDajjal ? DajjalDropChance : ShaytaanDropChance;
	float BookChance = bFromDajjal ? DajjalBookChance : ShaytaanBookChance;
#if !UE_BUILD_SHIPPING
	// Dev only: -LWDrops makes every kill drop something, and a book a quarter of the time.
	static const bool bDevDrops = FParse::Param(FCommandLine::Get(), TEXT("LWDrops"));
	if (bDevDrops)
	{
		DropChance = 1.f;
		BookChance = 0.25f;
		bBookDroppedThisWave = false;
	}
#endif

	// The book is rare and only one can drop per wave.
	EPickupType Type;
	if (!bBookDroppedThisWave && FMath::FRand() < BookChance)
	{
		Type = EPickupType::Book;
		bBookDroppedThisWave = true;
	}
	else if (FMath::FRand() < DropChance)
	{
		Type = static_cast<EPickupType>(FMath::RandRange(0, static_cast<int32>(EPickupType::Shield)));
	}
	else
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, FVector(Location.X, Location.Y, Arena->GetCentre().Z + PlaneHeight), FRotator::ZeroRotator, Params))
	{
		Pickup->InitPickup(Type, GetLightWalker());
	}
}
