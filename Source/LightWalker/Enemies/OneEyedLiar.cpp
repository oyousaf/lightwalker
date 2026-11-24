#include "Enemies/OneEyedLiar.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWFeel.h"
#include "Core/LWGameMode.h"
#include "Core/LWNeon.h"
#include "Core/LWSound.h"
#include "Effects/LWParticles.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Player/LightWalkerPawn.h"

namespace
{
	constexpr int32 MaxBolts = 320;
	constexpr float BeamLength = 4200.f;
	constexpr float BossSize = 560.f;
	constexpr float BossLift = 150.f;
	const FTransform HiddenInstance(FQuat::Identity, FVector(0.f, 0.f, -5000.f), FVector(0.001f));
}

// ---------------------------------------------------------------------------------------------------------------
// Guardians

ALiarGuardian::ALiarGuardian()
{
	HitPoints = 5;
	Radius = 62.f;
	ModelSize = 130.f;
}

void ALiarGuardian::UpdateMovement(float DeltaSeconds)
{
	const AOneEyedLiar* Master = Boss.Get();
	if (!Master || Master->IsDead())
	{
		return;
	}
	// Spin out from the boss and circle it.
	Out = FMath::FInterpTo(Out, OrbitRadius, DeltaSeconds, 2.5f);
	Angle += OrbitRate * DeltaSeconds;
	const FVector At = Master->GetActorLocation() + FRotator(0.f, Angle, 0.f).RotateVector(FVector(Out, 0.f, 0.f));
	SetActorLocation(FVector(At.X, At.Y, GetActorLocation().Z));
}

// ---------------------------------------------------------------------------------------------------------------
// The One-Eyed Liar

AOneEyedLiar::AOneEyedLiar()
{
	Radius = 215.f;
	ModelSize = BossSize;
	ModelGlow = 0.8f;
	HitSwell = 0.02f;
	HitGlow = 1.f;
	HitFlashTime = 0.12f;

	ModelPath = FSoftObjectPath(TEXT("/Game/Enemies/SM_OneEyedLiar.SM_OneEyedLiar"));
	ModelYaw = -90.f;

	BoltGlow = LWNeon::MakeInstances(this, TEXT("BoltGlow"), ELWShape::Sphere, RootComponent);
	BoltHeart = LWNeon::MakeInstances(this, TEXT("BoltHeart"), ELWShape::Sphere, RootComponent);
	GuardianRing = LWNeon::MakeInstances(this, TEXT("GuardianRing"), ELWShape::Cube, RootComponent);
	BeamGlow = LWNeon::MakeMesh(this, TEXT("BeamGlow"), ELWShape::Cube, RootComponent);
	BeamHeart = LWNeon::MakeMesh(this, TEXT("BeamHeart"), ELWShape::Cube, RootComponent);
	Charge = LWNeon::MakeMesh(this, TEXT("Charge"), ELWShape::Sphere, RootComponent);
	for (UStaticMeshComponent* Part : { static_cast<UStaticMeshComponent*>(BoltGlow), static_cast<UStaticMeshComponent*>(BoltHeart),
		static_cast<UStaticMeshComponent*>(GuardianRing), BeamGlow.Get(), BeamHeart.Get(), Charge.Get() })
	{
		// Placed in the world, not carried by the boss.
		LWNeon::SetWorldPlaced(Part);
	}
	for (UStaticMeshComponent* Part : { BeamGlow.Get(), BeamHeart.Get(), Charge.Get() })
	{
		Part->SetVisibility(false);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetRelativeLocation(FVector(-250.f, 0.f, 420.f));
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(120.f);
	Glow->SetAttenuationRadius(1500.f);
	Glow->SetLightColor(FLinearColor(1.f, 0.55f, 0.2f));
	Glow->SetCastShadows(false);
	Glow->SetIndirectLightingIntensity(0.f);
}

void AOneEyedLiar::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;

	for (int32 Index = 0; Index < MaxBolts; ++Index)
	{
		BoltGlow->AddInstance(HiddenInstance, true);
		BoltHeart->AddInstance(HiddenInstance, true);
	}
	for (int32 Index = 0; Index < 28; ++Index)
	{
		GuardianRing->AddInstance(HiddenInstance, true);
	}
	LWNeon::ApplyGlow(BoltGlow, FLinearColor(1.f, 0.04f, 0.2f), 2.6f);
	LWNeon::ApplyGlow(BoltHeart, FLinearColor::Black, 1.f);
	LWNeon::ApplyGlow(GuardianRing, FLinearColor(1.f, 0.55f, 0.08f), 3.f);
	LWNeon::ApplyGlow(BeamGlow, FLinearColor(1.f, 0.04f, 0.2f), 2.2f);
	LWNeon::ApplyGlow(BeamHeart, FLinearColor::Black, 1.f);
	LWNeon::ApplyGlow(Charge, FLinearColor(1.f, 0.1f, 0.2f), 6.f);
}

void AOneEyedLiar::Arrive(const FVector& ArenaCentre, float Seconds)
{
	Centre = ArenaCentre;
	ArriveTime = ArriveLeft = FMath::Max(Seconds, 0.1f);
	Visual->SetRelativeLocation(FVector(0.f, 0.f, 2600.f));
}

FVector2D AOneEyedLiar::GetEye() const
{
	// Out of the face that's turned to Lightwalker.
	return FVector2D(GetActorLocation()) + FVector2D(FRotator(0.f, Visual->GetComponentRotation().Yaw, 0.f).Vector()) * 140.f;
}

float AOneEyedLiar::AngleToPlayer() const
{
	const ALightWalkerPawn* LightWalker = GetPlayer();
	const FVector To = LightWalker ? LightWalker->GetActorLocation() - GetActorLocation() : FVector::BackwardVector;
	return FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X));
}

void AOneEyedLiar::EnterStage(int32 NewStage)
{
	Stage = NewStage;
	StageAge = 0.f;
	AttackIndex = 0;
	AttackWait = 2.4f;
	ChargeLeft = -1.f;
	SpiralLeft = 0.f;
	ClearAttacks();
	if (Stage == 2)
	{
		RaiseGuardians();
	}
	if (OnStage)
	{
		OnStage(Stage);
	}
}

void AOneEyedLiar::RaiseGuardians()
{
	if (!OnSpawn)
	{
		return;
	}
	ULWSoundSubsystem::Play(this, ELWSound::DarkBolt);
	for (int32 Index = 0; Index < GuardianCount; ++Index)
	{
		ALiarGuardian* Guardian = Cast<ALiarGuardian>(OnSpawn(ALiarGuardian::StaticClass(), GetActorLocation()));
		if (Guardian)
		{
			Guardian->SetBoss(this, 360.f * Index / GuardianCount);
			Guardians.Add(Guardian);
		}
	}
}

void AOneEyedLiar::TakeHit()
{
	if (bDead)
	{
		return;
	}
	if (IsGuarded() || !bFighting)
	{
		// Turned aside: a dull knock, no harm.
		ULWSoundSubsystem::Play(this, ELWSound::WallHit);
		return;
	}

	--Health;
	HitFlash = 1.f;
	ULWSoundSubsystem::Play(this, ELWSound::DajjalHit);
	ULWFeelSubsystem::Play(this, ELWFeel::BossHit);
	if (Health <= 0)
	{
		Kill();
		return;
	}
	const int32 Wanted = GetHealth() <= StageThreeAt ? 3 : (GetHealth() <= StageTwoAt ? 2 : 1);
	if (Wanted > Stage)
	{
		EnterStage(Wanted);
	}
}

void AOneEyedLiar::Kill()
{
	if (bDead)
	{
		return;
	}
	// It doesn't go all at once (UpdateDying).
	bDead = true;
	Health = 0;
	DyingTime = 0.f;
	ClearAttacks();
	ULWSoundSubsystem::Play(this, ELWSound::EyeBreak);
	ULWFeelSubsystem::Play(this, ELWFeel::BossDown);
	if (OnDefeated)
	{
		OnDefeated();
	}
}

void AOneEyedLiar::ClearAttacks()
{
	Bolts.Reset();
	Beam.State = FBossBeam::EState::Off;
	SpiralLeft = 0.f;
	ChargeLeft = -1.f;
	AttackWait = FMath::Max(AttackWait, 1.6f);
}

void AOneEyedLiar::AddBolt(float AngleDegrees, float Speed)
{
	if (Bolts.Num() >= MaxBolts)
	{
		return;
	}
	FBossBolt& Bolt = Bolts.AddDefaulted_GetRef();
	const float Angle = FMath::DegreesToRadians(AngleDegrees);
	const FVector2D Way(FMath::Cos(Angle), FMath::Sin(Angle));
	Bolt.Position = FVector2D(GetActorLocation()) + Way * 205.f;
	Bolt.Velocity = Way * Speed;
}

void AOneEyedLiar::ChooseAttack()
{
	// Each stage runs through its own round of attacks.
	static const TArray<EAttack> Rounds[] = {
		{ EAttack::Fan, EAttack::Ring, EAttack::Summon, EAttack::Fan, EAttack::Ring },
		{ EAttack::Beam, EAttack::Ring, EAttack::Fan, EAttack::Summon },
		{ EAttack::Spiral, EAttack::Fan, EAttack::Beam, EAttack::Summon, EAttack::Ring },
	};
	const TArray<EAttack>& Round = Rounds[FMath::Clamp(Stage, 1, 3) - 1];
	Current = Round[AttackIndex++ % Round.Num()];
	// The beam carries its own warning line.
	ChargeLeft = Current == EAttack::Beam ? 0.05f : 0.55f;
}

void AOneEyedLiar::BeginAttack(EAttack Attack)
{
	const float ToPlayer = AngleToPlayer();
	switch (Attack)
	{
	case EAttack::Fan:
	{
		const int32 Count = Stage == 1 ? 7 : 9;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			AddBolt(ToPlayer + (Index - (Count - 1) * 0.5f) * 11.f, 640.f);
		}
		ULWSoundSubsystem::Play(this, ELWSound::DarkBolt);
		break;
	}
	case EAttack::Ring:
	{
		// All round, with a way through.
		const int32 Count = Stage == 1 ? 18 : 24;
		const float Spin = FMath::FRandRange(0.f, 360.f);
		const int32 Way = FMath::RandRange(0, Count - 1);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (Index != Way && Index != (Way + 1) % Count)
			{
				AddBolt(Spin + 360.f * Index / Count, 430.f);
			}
		}
		ULWSoundSubsystem::Play(this, ELWSound::DarkBolt);
		break;
	}
	case EAttack::Summon:
		if (OnSummon)
		{
			OnSummon(Stage == 3 ? 4 : 6, Stage == 3 ? 1 : 0);
		}
		break;
	case EAttack::Beam:
	{
		// Starts to one side of Lightwalker and sweeps across.
		const float Side = FMath::RandBool() ? 1.f : -1.f;
		Beam.State = FBossBeam::EState::Warning;
		Beam.Age = 0.f;
		Beam.Angle = ToPlayer - Side * 38.f;
		Beam.SweepRate = Side * (Stage == 3 ? 36.f : 30.f);
		ULWSoundSubsystem::Play(this, ELWSound::WaveStart);
		break;
	}
	case EAttack::Spiral:
		SpiralLeft = 3.2f;
		SpiralTimer = 0.f;
		SpiralAngle = ToPlayer + 90.f;
		break;
	}

	// The rest before the next one.
	const float Rest = Stage == 1 ? 1.9f : (Stage == 2 ? 2.1f : 1.5f);
	AttackWait = Rest + (Attack == EAttack::Beam ? BeamWarning + BeamDuration : 0.f) + (Attack == EAttack::Spiral ? 3.2f : 0.f);
}

void AOneEyedLiar::UpdateAttack(float DeltaSeconds)
{
	if (ChargeLeft > 0.f)
	{
		ChargeLeft -= DeltaSeconds;
		const float T = FMath::Clamp(1.f - ChargeLeft / 0.55f, 0.f, 1.f);
		const FVector2D Eye = GetEye();
		Charge->SetVisibility(true);
		Charge->SetWorldLocation(FVector(Eye.X, Eye.Y, GetActorLocation().Z + BossLift));
		Charge->SetWorldScale3D(FVector(0.2f + 0.9f * T));
		if (ChargeLeft <= 0.f)
		{
			Charge->SetVisibility(false);
			BeginAttack(Current);
		}
		return;
	}
	Charge->SetVisibility(false);

	if (SpiralLeft > 0.f)
	{
		SpiralLeft -= DeltaSeconds;
		SpiralTimer -= DeltaSeconds;
		while (SpiralTimer <= 0.f)
		{
			SpiralTimer += 0.15f;
			SpiralAngle += 14.f;
			for (int32 Arm = 0; Arm < 4; ++Arm)
			{
				AddBolt(SpiralAngle + Arm * 90.f, 470.f);
			}
			ULWSoundSubsystem::Play(this, ELWSound::Shot);
		}
	}

	AttackWait -= DeltaSeconds;
	if (AttackWait <= 0.f)
	{
		ChooseAttack();
	}
}

void AOneEyedLiar::UpdateMovement(float DeltaSeconds)
{
	bFighting = true;
	StageAge += DeltaSeconds;

	// The guardians: gone for good ones drop out of the list, and once all are down it's open for a while.
	if (Stage == 2)
	{
		const bool bWasGuarded = Guardians.Num() > 0;
		Guardians.RemoveAll([](const ALiarGuardian* Guardian) { return !IsValid(Guardian) || Guardian->IsDead(); });
		if (bWasGuarded && Guardians.Num() == 0)
		{
			OpenLeft = OpenTime;
			GlowPulse = 1.f;
			ULWSoundSubsystem::Play(this, ELWSound::EyeBreak);
		}
		else if (Guardians.Num() == 0)
		{
			OpenLeft -= DeltaSeconds;
			if (OpenLeft <= 0.f)
			{
				RaiseGuardians();
			}
		}
	}
	else if (Guardians.Num() > 0)
	{
		Guardians.Reset();
	}

	FVector Wanted;
	float Speed = 260.f;
	switch (Stage)
	{
	case 1:
		Wanted = Centre + FVector(520.f + FMath::Sin(StageAge * 0.8f) * 110.f, FMath::Sin(StageAge * 0.45f) * 880.f, 0.f);
		break;
	case 2:
		Wanted = Centre + FVector(FMath::Sin(StageAge * 0.5f) * 80.f, FMath::Cos(StageAge * 0.37f) * 160.f, 0.f);
		break;
	default:
		// After Lightwalker, unless it's busy with its beam.
		Wanted = GetPlayer() && !GetPlayer()->IsDowned() ? GetPlayer()->GetActorLocation() : Centre;
		Speed = Beam.State == FBossBeam::EState::Off ? 150.f : 0.f;
		break;
	}
	FVector Location = FMath::VInterpConstantTo(GetActorLocation(), FVector(Wanted.X, Wanted.Y, GetActorLocation().Z), DeltaSeconds, Speed);
	if (Bounds.bIsValid)
	{
		Location.X = FMath::Clamp(Location.X, Bounds.Min.X + Radius * 0.6f, Bounds.Max.X - Radius * 0.6f);
		Location.Y = FMath::Clamp(Location.Y, Bounds.Min.Y + Radius * 0.6f, Bounds.Max.Y - Radius * 0.6f);
	}
	SetActorLocation(Location);
	UpdateAttack(DeltaSeconds);
}

void AOneEyedLiar::UpdateLook(float DeltaSeconds)
{
	if (IsDying())
	{
		return;
	}
	if (const ALightWalkerPawn* LightWalker = GetPlayer())
	{
		TurnVisualTowards(LightWalker->GetActorLocation() - GetActorLocation(), DeltaSeconds, 3.f);
	}

	// Comes down out of the dark, then hovers.
	ArriveLeft = FMath::Max(0.f, ArriveLeft - DeltaSeconds);
	const float T = ArriveLeft / ArriveTime;
	const float Hover = BossLift + FMath::Sin(LookTime * 1.4f) * 18.f;
	const bool bWasAbove = Visual->GetRelativeLocation().Z > BossLift + 60.f;
	Visual->SetRelativeLocation(FVector(0.f, 0.f, Hover + 2400.f * T * T * T));
	if (bWasAbove && T <= 0.28f && T > 0.f && T + DeltaSeconds / ArriveTime > 0.28f)
	{
		ULWFeelSubsystem::Play(this, ELWFeel::BossDown);
	}

	GlowPulse = FMath::Max(0.f, GlowPulse - DeltaSeconds * 0.8f);
	SetModelGlow((Stage == 3 ? 1.05f : 0.8f) * (IsGuarded() ? 0.55f : 1.f) + GlowPulse * 2.f);
	Glow->SetLightColor(Stage == 3 ? FLinearColor(1.f, 0.18f, 0.1f) : FLinearColor(1.f, 0.55f, 0.2f));

	// The ring of light that shows it's guarded.
	if (IsGuarded() || bRingShown)
	{
		bRingShown = IsGuarded();
		TArray<FTransform> Ring;
		const int32 Count = GuardianRing->GetInstanceCount();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FRotator Turn(0.f, 360.f * Index / Count - LookTime * 40.f, 0.f);
			Ring.Add(bRingShown ? FTransform(Turn, GetActorLocation() + Turn.RotateVector(FVector(300.f, 0.f, 10.f)), FVector(0.08f, 0.38f, 0.08f)) : HiddenInstance);
		}
		GuardianRing->BatchUpdateInstancesTransforms(0, Ring, true, true, true);
	}
}

void AOneEyedLiar::UpdateBolts(float DeltaSeconds, bool bCanHurt)
{
	ALightWalkerPawn* LightWalker = GetPlayer();
	const FVector2D PlayerAt = LightWalker ? FVector2D(LightWalker->GetActorLocation()) : FVector2D::ZeroVector;
	// A little kinder than it looks: only the middle of Lightwalker counts.
	const float HitDistance = BoltRadius + (LightWalker ? LightWalker->GetContactRadius() * 0.5f : 0.f);
	bool bHit = false;
	for (int32 Index = Bolts.Num() - 1; Index >= 0; --Index)
	{
		FBossBolt& Bolt = Bolts[Index];
		Bolt.Position += Bolt.Velocity * DeltaSeconds;
		if (bCanHurt && LightWalker && LightWalker->IsVulnerable() && FVector2D::DistSquared(Bolt.Position, PlayerAt) < FMath::Square(HitDistance))
		{
			bHit = true;
			Bolts.RemoveAtSwap(Index);
			continue;
		}
		if (Bounds.bIsValid && !Bounds.ExpandBy(140.f).IsInside(Bolt.Position))
		{
			Bolts.RemoveAtSwap(Index);
		}
	}

	const float Throb = 1.f + 0.1f * FMath::Sin(LookTime * 16.f);
	const float Z = GetActorLocation().Z;
	// Only the bolts in the air, and those that just went out, need moving.
	const int32 Count = FMath::Max(Bolts.Num(), BoltsShown);
	BoltsShown = Bolts.Num();
	TArray<FTransform> Glows;
	TArray<FTransform> Hearts;
	Glows.Reserve(Count);
	Hearts.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Bolts.IsValidIndex(Index))
		{
			const FVector At(Bolts[Index].Position.X, Bolts[Index].Position.Y, Z);
			Glows.Emplace(FQuat::Identity, At, FVector(0.62f * Throb));
			// Set towards the camera, so the dark heart shows inside the glow.
			Hearts.Emplace(FQuat::Identity, At + FVector(-14.f, 0.f, 24.f), FVector(0.34f));
		}
		else
		{
			Glows.Add(HiddenInstance);
			Hearts.Add(HiddenInstance);
		}
	}
	if (Count > 0)
	{
		BoltGlow->BatchUpdateInstancesTransforms(0, Glows, true, true, true);
		BoltHeart->BatchUpdateInstancesTransforms(0, Hearts, true, true, true);
	}

	if (bHit)
	{
		if (ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>())
		{
			GameMode->HandlePlayerTouched(LightWalker);
		}
	}
}

void AOneEyedLiar::UpdateBeam(float DeltaSeconds, bool bCanHurt)
{
	const bool bShown = Beam.State != FBossBeam::EState::Off;
	BeamGlow->SetVisibility(bShown);
	BeamHeart->SetVisibility(Beam.State == FBossBeam::EState::Firing);
	if (!bShown)
	{
		return;
	}

	Beam.Age += DeltaSeconds;
	if (Beam.State == FBossBeam::EState::Warning && Beam.Age >= BeamWarning)
	{
		Beam.State = FBossBeam::EState::Firing;
		Beam.Age = 0.f;
		ULWSoundSubsystem::Play(this, ELWSound::BeamLost);
		ULWFeelSubsystem::Play(this, ELWFeel::DajjalKill);
	}
	else if (Beam.State == FBossBeam::EState::Firing)
	{
		Beam.Angle += Beam.SweepRate * DeltaSeconds;
		if (Beam.Age >= BeamDuration)
		{
			Beam.State = FBossBeam::EState::Off;
			return;
		}
	}

	const FRotator Turn(0.f, Beam.Angle, 0.f);
	const FVector Along = Turn.Vector();
	const FVector Start = GetActorLocation();
	const FVector Middle = Start + Along * (BeamLength * 0.5f);
	const bool bFiring = Beam.State == FBossBeam::EState::Firing;
	const float Width = bFiring ? BeamHalfWidth * 2.f * (1.f + 0.06f * FMath::Sin(LookTime * 40.f)) : 7.f + 5.f * FMath::Sin(LookTime * 30.f);
	BeamGlow->SetWorldLocationAndRotation(Middle, Turn);
	BeamGlow->SetWorldScale3D(FVector(BeamLength, Width, bFiring ? 36.f : 4.f) / 100.f);
	BeamHeart->SetWorldLocationAndRotation(Middle + FVector(0.f, 0.f, 8.f), Turn);
	BeamHeart->SetWorldScale3D(FVector(BeamLength, Width * 0.55f, 34.f) / 100.f);

	ALightWalkerPawn* LightWalker = GetPlayer();
	if (bFiring && bCanHurt && LightWalker && LightWalker->IsVulnerable())
	{
		const FVector To = LightWalker->GetActorLocation() - Start;
		const float Forward = FVector::DotProduct(To, Along);
		const float Sideways = FMath::Abs(FVector::CrossProduct(Along, To).Z);
		if (Forward > 0.f && Sideways < BeamHalfWidth + LightWalker->GetContactRadius() * 0.5f)
		{
			if (ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>())
			{
				GameMode->HandlePlayerTouched(LightWalker);
			}
		}
	}
}

void AOneEyedLiar::UpdateDying(float DeltaSeconds)
{
	DyingTime += DeltaSeconds;
	const float T = FMath::Clamp(DyingTime / DyingDuration, 0.f, 1.f);

	// Shudders, sinks and burns brighter and brighter, with bursts breaking out of it, then goes.
	const float Shake = 10.f + 26.f * T;
	Visual->SetRelativeLocation(FVector(FMath::FRandRange(-Shake, Shake), FMath::FRandRange(-Shake, Shake), BossLift * (1.f - 0.7f * T)));
	Visual->AddLocalRotation(FRotator(0.f, (60.f + 500.f * T * T) * DeltaSeconds, 0.f));
	Visual->SetRelativeScale3D(FVector(1.f - 0.35f * T * T));
	SetModelGlow(0.9f + 9.f * T * T);
	Glow->SetIntensity(120.f + 900.f * T * T);

	DyingBurst -= DeltaSeconds;
	if (DyingBurst <= 0.f)
	{
		DyingBurst = FMath::Lerp(0.32f, 0.1f, T);
		LWFX::DajjalDeath(GetWorld(), GetActorLocation() + FVector(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(20.f, 220.f)));
		ULWSoundSubsystem::Play(this, ELWSound::DajjalDeath);
		ULWFeelSubsystem::Play(this, ELWFeel::Kill);
	}
	if (T < 1.f)
	{
		return;
	}

	FLWEmit Flash;
	Flash.Count = 1;
	Flash.Origin = GetActorLocation() + FVector(0.f, 0.f, 80.f);
	Flash.Speed = FVector2D::ZeroVector;
	Flash.Life = FVector2D(0.7f, 0.7f);
	Flash.Size = FVector2D(11.f, 11.f);
	Flash.bFlash = true;
	FLWEmit Light;
	Light.Count = 150;
	Light.Origin = Flash.Origin;
	Light.Speed = FVector2D(300.f, 2300.f);
	Light.Life = FVector2D(0.8f, 2.2f);
	Light.Size = FVector2D(0.06f, 0.2f);
	Light.Drag = 1.6f;
	Light.Gravity = -60.f;
	LWFX::Burst(GetWorld(), ELWShape::Sphere, FLinearColor(1.f, 0.85f, 0.45f), 9.f, { Flash, Light });
	FLWEmit Shards;
	Shards.Count = 90;
	Shards.Origin = Flash.Origin;
	Shards.OriginRadius = 160.f;
	Shards.Speed = FVector2D(400.f, 1700.f);
	Shards.Life = FVector2D(0.9f, 1.8f);
	Shards.Size = FVector2D(0.12f, 0.42f);
	Shards.Drag = 1.8f;
	Shards.Gravity = 900.f;
	Shards.Lift = 520.f;
	Shards.bSpin = true;
	LWFX::Burst(GetWorld(), ELWShape::Cube, FLinearColor(1.f, 0.6f, 0.15f), 5.f, { Shards });
	ULWSoundSubsystem::Play(this, ELWSound::Gate);
	ULWFeelSubsystem::Play(this, ELWFeel::BossDown);
	if (OnGone)
	{
		OnGone();
	}
	Destroy();
}

void AOneEyedLiar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsDying())
	{
		UpdateBolts(DeltaSeconds, false);
		UpdateBeam(DeltaSeconds, false);
		UpdateDying(DeltaSeconds);
		return;
	}
	UpdateBolts(DeltaSeconds, bFighting);
	UpdateBeam(DeltaSeconds, bFighting);
}
