#include "Shield/ShieldField.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Core/LWSound.h"
#include "Effects/LWParticles.h"
#include "Enemies/EnemyBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LightWalker.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Shield/ShieldPawn.h"
#include "UI/LWStyle.h"

namespace
{
	constexpr float ColumnWidth = 200.f;
	constexpr float RowHeight = 128.f;
	constexpr float BlockHeight = 62.f;
	constexpr int32 WallColumns = 14;
	constexpr float RimGlow = 0.17f;
	constexpr float FaceGlow = 1.4f;

	/**
	 * The walls, top row first, 14 blocks across:
	 *   # shadow (1 hit)   H hard (2)   S stone (3)   V power-up   B book   C caged shaytaan   D dajjal's eye (2)
	 *   O the great eye (covers 4 x 2 from here)
	 */
	const TArray<TArray<const TCHAR*>>& Walls()
	{
		static const TArray<TArray<const TCHAR*>> All = {
			// The Veil.
			{
				TEXT("..##########.."),
				TEXT(".##HH####HH##."),
				TEXT("####V####V####"),
				TEXT(".############."),
				TEXT("..##..##..##.."),
			},
			// The Keep: a hard outer wall, cages and watching eyes inside it.
			{
				TEXT("HHHHHHBHHHHHHH"),
				TEXT("H.C..#VV#..C.H"),
				TEXT("H.##.####.##.H"),
				TEXT("D....#..#....D"),
				TEXT("##SS##..##SS##"),
				TEXT(".##..#VV#..##."),
			},
			// The Eye of Horus.
			{
				TEXT("...########..."),
				TEXT(".###SSSSSS###."),
				TEXT("##VS.O....SV##"),
				TEXT("##.S......S.##"),
				TEXT(".###SS..SS###."),
				TEXT("...###CC###..."),
				TEXT(".....#VV#....."),
			},
		};
		return All;
	}

	FLinearColor ColorFor(EWallBlock Kind)
	{
		switch (Kind)
		{
		case EWallBlock::Hard:     return FLinearColor(0.35f, 0.2f, 1.f);
		case EWallBlock::Stone:    return FLinearColor(0.12f, 0.3f, 1.f);
		case EWallBlock::Vessel:   return FLinearColor(0.2f, 1.f, 0.6f);
		case EWallBlock::Book:     return FLinearColor(1.f, 0.7f, 0.15f);
		case EWallBlock::Cage:     return FLinearColor(1.f, 0.08f, 0.06f);
		case EWallBlock::Eye:
		case EWallBlock::GreatEye: return FLinearColor(1.f, 0.6f, 0.12f);
		default:                   return FLinearColor(0.8f, 0.1f, 0.75f);
		}
	}

	bool IsEye(EWallBlock Kind)
	{
		return Kind == EWallBlock::Eye || Kind == EWallBlock::GreatEye;
	}

	const TCHAR* ArtFor(EWallBlock Kind, bool bCracked)
	{
		switch (Kind)
		{
		case EWallBlock::Hard:     return TEXT("BlockHard");
		case EWallBlock::Stone:    return TEXT("BlockStone");
		case EWallBlock::Vessel:   return TEXT("BlockVessel");
		case EWallBlock::Book:     return TEXT("BlockBook");
		case EWallBlock::Cage:     return TEXT("BlockCage");
		case EWallBlock::Eye:      return bCracked ? TEXT("BlockEyeBroken") : TEXT("BlockEye");
		case EWallBlock::GreatEye: return bCracked ? TEXT("BlockGreatEyeBroken") : TEXT("BlockGreatEye");
		default:                   return TEXT("BlockShadow");
		}
	}

	int32 HitsFor(EWallBlock Kind)
	{
		switch (Kind)
		{
		case EWallBlock::Hard:
		case EWallBlock::Eye:      return 2;
		case EWallBlock::Stone:    return 3;
		case EWallBlock::GreatEye: return 12;
		default:                   return 1;
		}
	}
}

AShieldField::AShieldField()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

int32 AShieldField::GetWallCount()
{
	return Walls().Num();
}

void AShieldField::Setup(AShieldPawn* InPlayer, const FBox2D& InBounds, float InPlaneZ)
{
	Player = InPlayer;
	Bounds = InBounds;
	PlaneZ = InPlaneZ;
	if (Player)
	{
		// The shield moves first, so the beam always meets it where it's drawn.
		AddTickPrerequisiteActor(Player);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// The wall

void AShieldField::AddBlock(EWallBlock Kind, float Column, float Row, float AcrossColumns, float DownRows)
{
	FWallBlock Block;
	Block.Kind = Kind;
	Block.Half = FVector2D(RowHeight * DownRows * 0.5f - 7.f, ColumnWidth * AcrossColumns * 0.5f - 7.f);
	Block.Centre = FVector2D(
		Bounds.Max.X - 150.f - RowHeight * (Row + DownRows * 0.5f),
		(Bounds.Min.Y + Bounds.Max.Y) * 0.5f + ColumnWidth * (Column + AcrossColumns * 0.5f - WallColumns * 0.5f));
	Block.HitsLeft = HitsFor(Kind);
	Block.Color = ColorFor(Kind);
	Block.FireTimer = FMath::FRandRange(EyeInterval.X, EyeInterval.Y);
	Block.RiseDelay = 0.05f * FMath::Abs(Column + AcrossColumns * 0.5f - WallColumns * 0.5f) + 0.06f * Row;
	BuildBlockMeshes(Block);
	Blocks.Add(MoveTemp(Block));
}

void AShieldField::BuildBlockMeshes(FWallBlock& Block)
{
	const FVector Base(Block.Centre.X, Block.Centre.Y, 0.f);
	const FVector Size = FVector(Block.Half.X * 2.f, Block.Half.Y * 2.f, BlockHeight) / 100.f;
	auto Keep = [&Block](UStaticMeshComponent* Part)
	{
		Block.Parts.Add(Part);
		Block.PartScales.Add(Part->GetComponentScale());
		return Part;
	};

	UStaticMeshComponent* Body = Keep(LWNeon::AddMesh(this, ELWShape::Cube, RootComponent, Base + FVector(0.f, 0.f, BlockHeight * 0.5f), Size * 0.98f));
	Block.Rim = LWNeon::ApplyGlow(Body, Block.Color, RimGlow);

	// The art lies flat on top, the right way up on screen (across is +Y, down is -X).
	const FRotator Flat(0.f, 90.f, 0.f);
	const FVector2D FaceSize(Block.Half.Y * 2.f, Block.Half.X * 2.f);
	if (UStaticMeshComponent* Face = LWNeon::AddCard(this, LWStyle::Art(ArtFor(Block.Kind, false)), RootComponent, Base + FVector(0.f, 0.f, BlockHeight + 1.f), FaceSize, Flat, FaceGlow))
	{
		Keep(Face);
		Block.Face = Cast<UMaterialInstanceDynamic>(Face->GetMaterial(0));
	}
	if (IsEye(Block.Kind))
	{
		const float Width = Block.Kind == EWallBlock::GreatEye ? 120.f : 50.f;
		if (UStaticMeshComponent* Iris = LWNeon::AddCard(this, LWStyle::Art(TEXT("BlockIris")), RootComponent, Base + FVector(0.f, 0.f, BlockHeight + 2.f), FVector2D(Width), Flat, FaceGlow))
		{
			Iris->SetTranslucentSortPriority(1);  // in front of the eye
			Block.Iris = Keep(Iris);
		}
	}

	// They rise into place (UpdateBlocks).
	for (TWeakObjectPtr<USceneComponent>& Part : Block.Parts)
	{
		Part->SetWorldScale3D(FVector(0.001f));
	}
}

void AShieldField::BuildWall(int32 Index)
{
	Clear();
	WallIndex = FMath::Clamp(Index, 0, GetWallCount() - 1);
	WallAge = 0.f;
	bCleared = false;
	Speed = BeamSpeed * (1.f + 0.07f * WallIndex);

	const TArray<const TCHAR*>& Layout = Walls()[WallIndex];
	for (int32 Row = 0; Row < Layout.Num(); ++Row)
	{
		for (int32 Column = 0; Layout[Row][Column]; ++Column)
		{
			switch (Layout[Row][Column])
			{
			case TEXT('#'): AddBlock(EWallBlock::Shadow, Column, Row); break;
			case TEXT('H'): AddBlock(EWallBlock::Hard, Column, Row); break;
			case TEXT('S'): AddBlock(EWallBlock::Stone, Column, Row); break;
			case TEXT('V'): AddBlock(EWallBlock::Vessel, Column, Row); break;
			case TEXT('B'): AddBlock(EWallBlock::Book, Column, Row); break;
			case TEXT('C'): AddBlock(EWallBlock::Cage, Column, Row); break;
			case TEXT('D'): AddBlock(EWallBlock::Eye, Column, Row); break;
			case TEXT('O'): AddBlock(EWallBlock::GreatEye, Column, Row, 4.f, 2.f); break;
			default: break;
			}
		}
	}
	StartingBlocks = FMath::Max(1, Blocks.Num());
}

bool AShieldField::HasGreatEye() const
{
	return Blocks.ContainsByPredicate([](const FWallBlock& Block) { return Block.Kind == EWallBlock::GreatEye; });
}

float AShieldField::GetWallLeft() const
{
	for (const FWallBlock& Block : Blocks)
	{
		if (Block.Kind == EWallBlock::GreatEye)
		{
			return static_cast<float>(Block.HitsLeft) / HitsFor(Block.Kind);
		}
	}
	return static_cast<float>(Blocks.Num()) / StartingBlocks;
}

void AShieldField::DestroyBlock(int32 Index, bool bBurst)
{
	FWallBlock& Block = Blocks[Index];
	if (bBurst)
	{
		const bool bGreat = Block.Kind == EWallBlock::GreatEye;
		FLWEmit Shards;
		Shards.Count = bGreat ? 60 : 10;
		Shards.Origin = FVector(Block.Centre.X, Block.Centre.Y, PlaneZ);
		Shards.OriginRadius = bGreat ? 220.f : 60.f;
		Shards.Speed = bGreat ? FVector2D(300.f, 1200.f) : FVector2D(150.f, 520.f);
		Shards.Life = bGreat ? FVector2D(0.6f, 1.4f) : FVector2D(0.3f, 0.65f);
		Shards.Size = bGreat ? FVector2D(0.1f, 0.3f) : FVector2D(0.06f, 0.15f);
		Shards.Drag = 2.6f;
		Shards.Gravity = 700.f;
		Shards.Lift = 260.f;
		Shards.bSpin = true;
		LWFX::Burst(GetWorld(), ELWShape::Cube, Block.Color, 4.f, { Shards });
	}
	for (TWeakObjectPtr<USceneComponent>& Part : Block.Parts)
	{
		if (Part.IsValid())
		{
			Part->DestroyComponent();
		}
	}
	Blocks.RemoveAt(Index);
}

void AShieldField::Clear()
{
	while (Blocks.Num() > 0)
	{
		DestroyBlock(Blocks.Num() - 1, false);
	}
	ClearBeams();
	ClearBolts();
}

// ---------------------------------------------------------------------------------------------------------------
// Beams

FLightBeam& AShieldField::AddBeam(const FVector2D& Position, const FVector2D& Direction)
{
	FLightBeam& Beam = Beams.AddDefaulted_GetRef();
	Beam.Position = Position;
	Beam.Direction = Direction.GetSafeNormal();

	const FVector At(Position.X, Position.Y, PlaneZ);
	UStaticMeshComponent* Core = LWNeon::AddMesh(this, ELWShape::Sphere, RootComponent, At, FVector(BeamRadius * 2.f / 100.f));
	LWNeon::ApplyGlow(Core, FLinearColor(1.f, 0.95f, 0.75f), 9.f);
	Beam.Core = Core;

	UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(Core);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(30.f);
	Light->SetAttenuationRadius(620.f);
	Light->SetLightColor(FLinearColor(1.f, 0.85f, 0.55f));
	Light->SetCastShadows(false);
	Light->SetIndirectLightingIntensity(0.f);
	Light->RegisterComponent();
	Beam.Light = Light;

	ULWParticleComponent* Trail = NewObject<ULWParticleComponent>(this);
	Trail->SetupAttachment(RootComponent);
	Trail->RegisterComponent();
	Trail->Setup(ELWShape::Sphere, FLinearColor(1.f, 0.75f, 0.3f), 7.f);
	Beam.Trail = Trail;
	return Beam;
}

void AShieldField::DestroyBeam(int32 Index)
{
	FLightBeam& Beam = Beams[Index];
	if (Beam.Light.IsValid()) { Beam.Light->DestroyComponent(); }
	if (Beam.Core.IsValid()) { Beam.Core->DestroyComponent(); }
	if (Beam.Trail.IsValid()) { Beam.Trail->DestroyComponent(); }
	Beams.RemoveAt(Index);
}

void AShieldField::ClearBeams()
{
	while (Beams.Num() > 0)
	{
		DestroyBeam(Beams.Num() - 1);
	}
}

void AShieldField::HoldBeam()
{
	if (Player)
	{
		Speed = BeamSpeed * (1.f + 0.07f * WallIndex);
		AddBeam(Player->GetShieldCentre() + Player->GetShieldFacing() * (BeamRadius + 8.f), Player->GetShieldFacing()).bHeld = true;
	}
}

bool AShieldField::HasHeldBeam() const
{
	return Beams.ContainsByPredicate([](const FLightBeam& Beam) { return Beam.bHeld; });
}

void AShieldField::SplitBeams()
{
	const int32 Count = Beams.Num();
	for (int32 Index = 0; Index < Count && Beams.Num() < 9; ++Index)
	{
		if (Beams[Index].bHeld)
		{
			continue;
		}
		for (const float Angle : { -22.f, 22.f })
		{
			// AddBeam can move the array, so copy what it needs.
			const FVector2D Position = Beams[Index].Position;
			const FVector2D Direction = Beams[Index].Direction.GetRotated(Angle);
			KeepSteep(AddBeam(Position, Direction));
		}
	}
}

void AShieldField::KeepSteep(FLightBeam& Beam) const
{
	// Never so flat that it just rattles between the sides.
	constexpr float Least = 0.45f;
	if (FMath::Abs(Beam.Direction.X) < Least)
	{
		Beam.Direction.X = Least * (Beam.Direction.X < 0.f ? -1.f : 1.f);
		Beam.Direction.Y = FMath::Sqrt(1.f - Least * Least) * (Beam.Direction.Y < 0.f ? -1.f : 1.f);
	}
	Beam.Direction.Normalize();
}

float AShieldField::ShieldDistance(const FVector2D& Point, float& OutAlong, FVector2D& OutNormal) const
{
	const FVector2D Facing = Player->GetShieldFacing();
	const FVector2D Side(-Facing.Y, Facing.X);
	const FVector2D From = Point - Player->GetShieldCentre();
	const float Half = Player->GetShieldHalfWidth();
	OutAlong = FMath::Clamp(static_cast<float>(FVector2D::DotProduct(From, Side)) / Half, -1.f, 1.f);
	const FVector2D OnArc = Side * (OutAlong * Half) - Facing * (OutAlong * OutAlong * Player->GetShieldBend());
	OutNormal = Facing.GetRotated(OutAlong * ShieldCurve);
	return FVector2D::Distance(From, OnArc);
}

void AShieldField::HitShield(FLightBeam& Beam)
{
	if (Beam.Direction.X > 0.f || Player->IsDowned())
	{
		return;
	}
	float Along;
	FVector2D Normal;
	if (ShieldDistance(Beam.Position, Along, Normal) > BeamRadius + 8.f || FVector2D::DotProduct(Beam.Direction, Normal) >= 0.f)
	{
		return;
	}

	Beam.Direction -= Normal * (2.f * FVector2D::DotProduct(Beam.Direction, Normal));
	Beam.Direction.X = FMath::Abs(Beam.Direction.X);
	KeepSteep(Beam);
	Beam.SinceShield = 0.f;
	Speed = FMath::Min(Speed * BeamSpeedUp, BeamTopSpeed);
	Player->FlashShield();

	FLWEmit Sparks;
	Sparks.Count = 12;
	Sparks.Origin = FVector(Beam.Position.X, Beam.Position.Y, PlaneZ);
	Sparks.Direction = FVector(Beam.Direction.X, Beam.Direction.Y, 0.f);
	Sparks.Spread = 50.f;
	Sparks.Speed = FVector2D(250.f, 700.f);
	Sparks.Life = FVector2D(0.15f, 0.35f);
	Sparks.Size = FVector2D(0.03f, 0.06f);
	Sparks.Drag = 5.f;
	LWFX::Burst(GetWorld(), ELWShape::Sphere, FLinearColor(0.5f, 0.95f, 1.f), 11.f, { Sparks });
	if (OnDeflect)
	{
		OnDeflect();
	}
}

void AShieldField::HitBlocks(FLightBeam& Beam)
{
	const bool bPiercing = Player->GetPowerUpTimeLeft(EPickupType::PiercingBeam) > 0.f;
	for (int32 Index = Blocks.Num() - 1; Index >= 0; --Index)
	{
		FWallBlock& Block = Blocks[Index];
		const FVector2D Near(
			FMath::Clamp(Beam.Position.X, Block.Centre.X - Block.Half.X, Block.Centre.X + Block.Half.X),
			FMath::Clamp(Beam.Position.Y, Block.Centre.Y - Block.Half.Y, Block.Centre.Y + Block.Half.Y));
		if (FVector2D::DistSquared(Near, Beam.Position) > FMath::Square(BeamRadius))
		{
			continue;
		}

		const bool bThrough = bPiercing && Block.Kind != EWallBlock::GreatEye;
		if (!bThrough)
		{
			// Bounce off the face it came through: the one it's least deep behind.
			const FVector2D Deep(
				Block.Half.X + BeamRadius - FMath::Abs(Beam.Position.X - Block.Centre.X),
				Block.Half.Y + BeamRadius - FMath::Abs(Beam.Position.Y - Block.Centre.Y));
			const FVector2D Away(Beam.Position.X < Block.Centre.X ? -1.f : 1.f, Beam.Position.Y < Block.Centre.Y ? -1.f : 1.f);
			if (Deep.X < Deep.Y)
			{
				Beam.Direction.X = FMath::Abs(Beam.Direction.X) * Away.X;
				Beam.Position.X += Deep.X * Away.X;
			}
			else
			{
				Beam.Direction.Y = FMath::Abs(Beam.Direction.Y) * Away.Y;
				Beam.Position.Y += Deep.Y * Away.Y;
			}
			KeepSteep(Beam);
		}

		Block.HitsLeft = bThrough ? 0 : Block.HitsLeft - 1;
		Block.Flash = 1.f;
		const bool bBroken = Block.HitsLeft <= 0;
		const FWallBlock Copy = Block;
		if (bBroken)
		{
			DestroyBlock(Index, true);
			if (Copy.Kind == EWallBlock::GreatEye)
			{
				// The whole wall goes with it.
				while (Blocks.Num() > 0)
				{
					DestroyBlock(Blocks.Num() - 1, true);
				}
			}
		}
		if (OnBlockHit)
		{
			OnBlockHit(Copy, bBroken);
		}
		return;  // one block a step
	}
}

void AShieldField::HitEnemies(const FLightBeam& Beam)
{
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && FVector2D::DistSquared(FVector2D(It->GetActorLocation()), Beam.Position) < FMath::Square(50.f + BeamRadius))
		{
			It->TakeHit();
			return;
		}
	}
}

void AShieldField::StepBeam(FLightBeam& Beam, float DeltaSeconds)
{
	Beam.Position += Beam.Direction * Speed * DeltaSeconds;
	Beam.SinceShield += DeltaSeconds;

	// The sides and the top.
	if (Beam.Position.Y < Bounds.Min.Y + BeamRadius)
	{
		Beam.Position.Y = Bounds.Min.Y + BeamRadius;
		Beam.Direction.Y = FMath::Abs(Beam.Direction.Y);
	}
	else if (Beam.Position.Y > Bounds.Max.Y - BeamRadius)
	{
		Beam.Position.Y = Bounds.Max.Y - BeamRadius;
		Beam.Direction.Y = -FMath::Abs(Beam.Direction.Y);
	}
	if (Beam.Position.X > Bounds.Max.X - BeamRadius)
	{
		Beam.Position.X = Bounds.Max.X - BeamRadius;
		Beam.Direction.X = -FMath::Abs(Beam.Direction.X);
	}

	// Caught in a loop: turn it a little.
	if (Beam.SinceShield > 9.f)
	{
		Beam.SinceShield = 6.f;
		Beam.Direction = Beam.Direction.GetRotated(FMath::RandBool() ? 13.f : -13.f);
		KeepSteep(Beam);
	}

	HitShield(Beam);
	HitBlocks(Beam);
}

bool AShieldField::PredictLanding(float LineX, float& OutY) const
{
	float Soonest = TNumericLimits<float>::Max();
	for (const FLightBeam& Beam : Beams)
	{
		if (Beam.bHeld)
		{
			continue;
		}
		// Going up, it has the wall to meet first: wait under it.
		if (Beam.Direction.X >= -0.01f)
		{
			if (Soonest == TNumericLimits<float>::Max())
			{
				OutY = Beam.Position.Y;
				Soonest = 1.0e6f;
			}
			continue;
		}
		const float Time = (Beam.Position.X - LineX) / (-Beam.Direction.X * Speed);
		if (Time > 0.f && Time < Soonest)
		{
			Soonest = Time;
			// Unfold the bounces off the sides.
			const float Width = Bounds.Max.Y - Bounds.Min.Y - BeamRadius * 2.f;
			float Y = Beam.Position.Y + Beam.Direction.Y * Speed * Time - (Bounds.Min.Y + BeamRadius);
			Y = FMath::Fmod(FMath::Fmod(Y, Width * 2.f) + Width * 2.f, Width * 2.f);
			OutY = Bounds.Min.Y + BeamRadius + (Y > Width ? Width * 2.f - Y : Y);
		}
	}
	return Soonest < TNumericLimits<float>::Max();
}

// ---------------------------------------------------------------------------------------------------------------
// Dark bolts

void AShieldField::DestroyBolt(int32 Index)
{
	if (Bolts[Index].Core.IsValid()) { Bolts[Index].Core->DestroyComponent(); }
	if (Bolts[Index].Halo.IsValid()) { Bolts[Index].Halo->DestroyComponent(); }
	Bolts.RemoveAt(Index);
}

void AShieldField::ClearBolts()
{
	while (Bolts.Num() > 0)
	{
		DestroyBolt(Bolts.Num() - 1);
	}
}

void AShieldField::UpdateBolts(float DeltaSeconds)
{
	for (FWallBlock& Block : Blocks)
	{
		if (!IsEye(Block.Kind))
		{
			continue;
		}
		Block.FireTimer -= DeltaSeconds;
		if (Block.FireTimer <= 0.f && !Player->IsDowned())
		{
			Block.FireTimer = FMath::FRandRange(EyeInterval.X, EyeInterval.Y);
			FDarkBolt& Bolt = Bolts.AddDefaulted_GetRef();
			Bolt.Position = Block.Centre - FVector2D(Block.Half.X + 20.f, 0.f);
			Bolt.Velocity = (FVector2D(Player->GetActorLocation()) - Bolt.Position).GetSafeNormal() * BoltSpeed;
			UStaticMeshComponent* Halo = LWNeon::AddMesh(this, ELWShape::Sphere, RootComponent, FVector(Bolt.Position.X, Bolt.Position.Y, PlaneZ), FVector(0.78f));
			LWNeon::ApplyGlow(Halo, FLinearColor(1.f, 0.04f, 0.2f), 2.6f);
			UStaticMeshComponent* Core = LWNeon::AddMesh(this, ELWShape::Sphere, RootComponent, FVector(Bolt.Position.X, Bolt.Position.Y, PlaneZ), FVector(0.42f));
			LWNeon::ApplyGlow(Core, FLinearColor(0.f, 0.f, 0.f), 1.f);
			Bolt.Halo = Halo;
			Bolt.Core = Core;
			ULWSoundSubsystem::Play(this, ELWSound::DarkBolt);
		}
	}

	for (int32 Index = Bolts.Num() - 1; Index >= 0; --Index)
	{
		FDarkBolt& Bolt = Bolts[Index];
		Bolt.Position += Bolt.Velocity * DeltaSeconds;
		const FVector At(Bolt.Position.X, Bolt.Position.Y, PlaneZ);
		const float Throb = 1.f + 0.12f * FMath::Sin(GetWorld()->GetTimeSeconds() * 18.f + Index);
		if (Bolt.Halo.IsValid())
		{
			Bolt.Halo->SetWorldLocation(At);
			Bolt.Halo->SetWorldScale3D(FVector(0.78f * Throb));
		}
		if (Bolt.Core.IsValid())
		{
			Bolt.Core->SetWorldLocation(At + FVector(-18.f, 0.f, 30.f));  // towards the camera, so the dark heart shows inside the glow
		}

		float Along;
		FVector2D Normal;
		if (!Player->IsDowned() && ShieldDistance(Bolt.Position, Along, Normal) < 30.f)
		{
			const FVector Where = At;
			DestroyBolt(Index);
			if (OnBoltBlocked)
			{
				OnBoltBlocked(Where);
			}
			continue;
		}
		if (Player->IsVulnerable() && FVector2D::Distance(Bolt.Position, FVector2D(Player->GetActorLocation())) < 25.f + Player->GetContactRadius())
		{
			DestroyBolt(Index);
			if (OnBoltHitPlayer)
			{
				OnBoltHitPlayer();
			}
			return;  // losing a life can clear the bolts
		}
		if (Bolt.Position.X < Bounds.Min.X - 200.f || FMath::Abs(Bolt.Position.Y) > Bounds.Max.Y + 400.f)
		{
			DestroyBolt(Index);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Running

void AShieldField::UpdateBlocks(float DeltaSeconds)
{
	WallAge += DeltaSeconds;
	for (FWallBlock& Block : Blocks)
	{
		const float Rise = FMath::Clamp((WallAge - Block.RiseDelay) / 0.45f, 0.f, 1.f);
		const bool bRising = WallAge < Block.RiseDelay + 0.6f;
		const bool bFlashing = Block.Flash > 0.f;
		Block.Flash = FMath::Max(0.f, Block.Flash - DeltaSeconds * 4.f);
		if (bRising || bFlashing)
		{
			// Springs up past full size and settles; swells a touch when hit.
			const float Spring = FMath::Max(0.001f, 1.f - FMath::Pow(1.f - Rise, 3.f) + 0.12f * FMath::Sin(Rise * PI)) * (1.f + 0.06f * Block.Flash);
			for (int32 Part = 0; Part < Block.Parts.Num(); ++Part)
			{
				if (Block.Parts[Part].IsValid())
				{
					Block.Parts[Part]->SetWorldScale3D(Block.PartScales[Part] * Spring);
				}
			}
		}
		// Worn blocks burn brighter as they're about to give.
		const float Worn = 1.f - static_cast<float>(Block.HitsLeft) / HitsFor(Block.Kind);
		const float Flash = Block.Flash * Block.Flash;
		if (bFlashing && Block.Rim.IsValid())
		{
			Block.Rim->SetScalarParameterValue(LWNeon::IntensityParam, RimGlow + Worn + 6.f * Flash);
		}
		if (bFlashing && Block.Face.IsValid())
		{
			Block.Face->SetScalarParameterValue(LWNeon::IntensityParam, FaceGlow + 0.5f * Worn + 4.f * Flash);
		}

		if (USceneComponent* Iris = Block.Iris.Get())
		{
			// An eye cracks when it's nearly done; until then it follows Lightwalker.
			if (!Block.bCracked && Worn >= 0.5f)
			{
				Block.bCracked = true;
				Iris->SetVisibility(false);
				if (Block.Face.IsValid())
				{
					Block.Face->SetTextureParameterValue(LWNeon::ArtParam, LWStyle::Art(ArtFor(Block.Kind, true)));
				}
			}
			const float Width = Block.Half.Y * 2.f;
			const float Look = FMath::Clamp((Player->GetActorLocation().Y - Block.Centre.Y) / 900.f, -1.f, 1.f);
			// The iris sits a touch right of the art's middle, and a touch above it.
			Iris->SetWorldLocation(FVector(Block.Centre.X + Block.Half.X * 0.04f, Block.Centre.Y + Width * (0.013f + 0.085f * Look), BlockHeight + 2.f));
		}
	}
}

void AShieldField::UpdateVisuals(float DeltaSeconds)
{
	for (FLightBeam& Beam : Beams)
	{
		if (Beam.bHeld)
		{
			Beam.Position = Player->GetShieldCentre() + Player->GetShieldFacing() * (BeamRadius + 8.f);
			Beam.Direction = Player->GetShieldFacing();
		}
		const FVector At(Beam.Position.X, Beam.Position.Y, PlaneZ);
		if (Beam.Core.IsValid())
		{
			// Drawn out along the way it's going, so it reads as a beam.
			Beam.Core->SetWorldLocation(At);
			Beam.Core->SetWorldRotation(FVector(Beam.Direction.X, Beam.Direction.Y, 0.f).Rotation());
			Beam.Core->SetWorldScale3D(FVector(Beam.bHeld ? 0.48f : 0.95f, 0.42f, 0.42f));
		}
		Beam.TrailTimer -= DeltaSeconds;
		if (!Beam.bHeld && Beam.TrailTimer <= 0.f && Beam.Trail.IsValid())
		{
			Beam.TrailTimer = 0.012f;
			FLWEmit Mote;
			Mote.Count = 1;
			Mote.Origin = At;
			Mote.OriginRadius = 6.f;
			Mote.Speed = FVector2D(5.f, 25.f);
			Mote.Life = FVector2D(0.25f, 0.4f);
			Mote.Size = FVector2D(0.14f, 0.2f);
			Mote.Drag = 1.f;
			Beam.Trail->Emit(Mote);
		}
	}
}

void AShieldField::CheckCleared()
{
	if (bCleared || Blocks.Num() > CollapseAt || (Blocks.Num() > 0 && HasGreatEye()))
	{
		return;
	}
	bCleared = true;
	while (Blocks.Num() > 0)
	{
		DestroyBlock(Blocks.Num() - 1, true);
	}
	if (OnWallCleared)
	{
		OnWallCleared();
	}
}

void AShieldField::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Player)
	{
		return;
	}

	DeltaSeconds = FMath::Min(DeltaSeconds, 1.f / 15.f);
	UpdateBlocks(DeltaSeconds);

	if (bRunning)
	{
		if (Player->WantsRelease())
		{
			for (FLightBeam& Beam : Beams)
			{
				if (Beam.bHeld)
				{
					// Straight where the shield faces, with a hair of angle so it never goes up and down one column.
					Beam.bHeld = false;
					Beam.Direction = Player->GetShieldFacing().GetRotated(FMath::FRandRange(4.f, 9.f) * (FMath::RandBool() ? 1.f : -1.f));
					ULWSoundSubsystem::Play(this, ELWSound::ShotPiercing);
				}
			}
		}

		// Steps of at most about 7 cm, so nothing is passed through.
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(DeltaSeconds * Speed / 7.f));
		for (int32 StepIndex = 0; StepIndex < Steps && bRunning; ++StepIndex)
		{
			for (int32 Index = Beams.Num() - 1; Index >= 0; --Index)
			{
				if (!Beams.IsValidIndex(Index) || Beams[Index].bHeld)
				{
					continue;
				}
				StepBeam(Beams[Index], DeltaSeconds / Steps);

				if (Beams.IsValidIndex(Index) && Beams[Index].Position.X < Bounds.Min.X - 70.f)
				{
					if (Beams.Num() == 1 && OnSaveBeam && OnSaveBeam())
					{
						Beams[Index].Position.X = Bounds.Min.X - 60.f;
						Beams[Index].Direction.X = FMath::Abs(Beams[Index].Direction.X);
						continue;
					}
					UE_LOG(LogLightWalker, Verbose, TEXT("[Shield] Beam fell at Y %.0f (direction %.2f %.2f, speed %.0f), shield at Y %.0f"),
						Beams[Index].Position.Y, Beams[Index].Direction.X, Beams[Index].Direction.Y, Speed, Player->GetActorLocation().Y);
					DestroyBeam(Index);
				}
			}
		}
		for (int32 Index = Beams.Num() - 1; Index >= 0; --Index)
		{
			if (Beams.IsValidIndex(Index) && !Beams[Index].bHeld)
			{
				HitEnemies(Beams[Index]);
			}
		}
		UpdateBolts(DeltaSeconds);
		if (bRunning)
		{
			CheckCleared();
		}
		if (bRunning && Beams.Num() == 0 && OnBeamsLost)
		{
			OnBeamsLost();
		}
	}
	UpdateVisuals(DeltaSeconds);
}
