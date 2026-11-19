#include "Arena/ArenaGameMode.h"

#include "Arena/Arena.h"
#include "Core/LightWalkerGameInstance.h"
#include "Enemies/Dajjal.h"
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

	DefaultWaves = MakeDefaultWaves();
	PickupClass = APickup::StaticClass();
}

const TArray<FArenaWave>& AArenaGameMode::GetWaves() const
{
	return (WaveSet && WaveSet->Waves.Num() > 0) ? WaveSet->Waves : DefaultWaves;
}

ULightWalkerGameInstance* AArenaGameMode::GetRun() const
{
	return GetGameInstance<ULightWalkerGameInstance>();
}

ALightWalkerPawn* AArenaGameMode::GetLightWalker() const
{
	return Cast<ALightWalkerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
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

	StartRun();

#if !UE_BUILD_SHIPPING
	if (FParse::Value(FCommandLine::Get(), TEXT("LWShots="), DevShotsRemaining) && DevShotsRemaining > 0)
	{
		GetWorldTimerManager().SetTimer(DevShotTimer, this, &AArenaGameMode::TakeDevScreenshot, 3.f, true, 8.f);
	}
#endif
}

#if !UE_BUILD_SHIPPING
void AArenaGameMode::TakeDevScreenshot()
{
	if (DevShotsRemaining-- <= 0)
	{
		FPlatformMisc::RequestExit(false);
		return;
	}
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		Controller->ConsoleCommand(FString::Printf(TEXT("HighResShot 2 filename=LWShot_%d"), ++DevShotIndex));
	}
}
#endif

void AArenaGameMode::StartRun()
{
	ClearField();
	GetRun()->StartRun(StartingLives);
	if (ALightWalkerPawn* LightWalker = GetLightWalker())
	{
		LightWalker->ClearPowerUps();
	}
	WaveIndex = 0;

#if !UE_BUILD_SHIPPING
	// Dev only: -LWStartWave=N skips straight to wave N.
	int32 StartWave = 1;
	if (FParse::Value(FCommandLine::Get(), TEXT("LWStartWave="), StartWave))
	{
		WaveIndex = FMath::Clamp(StartWave - 1, 0, FMath::Max(0, GetWaveCount() - 1));
	}
#endif
	BeginWaveIntro();
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
	SetPhase(EArenaPhase::WaveIntro, WaveIntroDuration, &AArenaGameMode::BeginFighting);
}

void AArenaGameMode::BeginFighting()
{
	SetEnemiesActive(true);
	SetPhase(EArenaPhase::Fighting, 0.f, nullptr);

#if !UE_BUILD_SHIPPING
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
		return;
	}

	LightWalker->SetDowned(true);
	LightWalker->ClearPowerUps();

	if (GetRun()->LoseLife() <= 0)
	{
		SetEnemiesActive(false);
		SetPhase(EArenaPhase::GameOver, 0.f, nullptr);
		return;
	}

	SetPhase(EArenaPhase::PlayerDown, RespawnDelay, &AArenaGameMode::RespawnLightWalker);
}

void AArenaGameMode::RespawnLightWalker()
{
	if (ALightWalkerPawn* LightWalker = GetLightWalker())
	{
		LightWalker->Revive(FindRespawnPoint(), RespawnGrace);
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
				Clearance = FMath::Min(Clearance, FVector2D::Distance(Point, FVector2D(Location.X, Location.Y)));
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
	SetPhase(EArenaPhase::WaveCleared, WaveClearedDuration, &AArenaGameMode::AdvanceWave);
}

void AArenaGameMode::AdvanceWave()
{
	++WaveIndex;
	if (GetWaves().IsValidIndex(WaveIndex))
	{
		BeginWaveIntro();
	}
	else
	{
		SetPhase(EArenaPhase::Victory, 0.f, nullptr);
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
