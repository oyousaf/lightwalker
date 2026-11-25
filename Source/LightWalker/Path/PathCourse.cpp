#include "Path/PathCourse.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Core/LWSeason.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/LWStyle.h"

namespace
{
	/** Half the depth of the course, towards and away from the camera. */
	constexpr float PathDepth = 130.f;
	constexpr float LedgeThickness = 36.f;
	/** The neon grid at the bottom of the abyss. */
	constexpr float AbyssZ = -1500.f;

	const FLinearColor GroundEdge(1.f, 0.1f, 0.45f);
	const FLinearColor LedgeEdge(0.2f, 0.85f, 1.f);
	const FLinearColor CrumbleEdge(1.f, 0.42f, 0.1f);
	const FLinearColor MovingEdge(0.75f, 0.2f, 1.f);
	const FLinearColor GateColor(1.f, 0.8f, 0.35f);

	FLinearColor EdgeColorFor(EPathBlock Kind)
	{
		switch (Kind)
		{
		case EPathBlock::Ledge:     return LedgeEdge;
		case EPathBlock::Crumbling: return CrumbleEdge;
		case EPathBlock::Moving:    return MovingEdge;
		default:                    return GroundEdge;
		}
	}
}

APathCourse::APathCourse()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->bConstrainAspectRatio = false;

	Moonlight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moonlight"));
	Moonlight->SetupAttachment(RootComponent);
	Moonlight->SetRelativeRotation(FRotator(-50.f, 25.f, 0.f));
	Moonlight->SetIntensity(MoonlightIntensity);
	Moonlight->SetLightColor(FLinearColor(0.45f, 0.5f, 1.f));
	Moonlight->SetCastShadows(false);

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;
	LWNeon::SetupPostProcess(PostProcess->Settings);

	Sky = CreateDefaultSubobject<USceneComponent>(TEXT("Sky"));
	Sky->SetupAttachment(RootComponent);
	Sun = CreateDefaultSubobject<USceneComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Sky);

	// Built here rather than in BeginPlay, so the game mode can read the layout as soon as the course exists.
	BuildCourse();
}

// ---------------------------------------------------------------------------------------------------------------
// The layout

void APathCourse::Ground(float Length)
{
	AddBlock(EPathBlock::Ground, 0.f, Length, 0.f);
	CursorY += Length;
}

void APathCourse::AddBlock(EPathBlock Kind, float FromCursor, float Width, float Height, const FVector2D& Travel, float Period)
{
	FPathBlock& Block = Blocks.AddDefaulted_GetRef();
	Block.Kind = Kind;
	Block.Left = CursorY + FromCursor;
	Block.Right = Block.Left + Width;
	Block.Top = CursorZ + Height;
	Block.Travel = Travel;
	Block.Period = Period;
}

void APathCourse::SpikesBack(float Back, float Width)
{
	Spikes.Add({ CursorY - Back, CursorY - Back + Width, CursorZ });
}

void APathCourse::Orbs(float FromCursor, int32 Count, float Span, float Height, float Arc)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float T = Count > 1 ? static_cast<float>(Index) / (Count - 1) : 0.5f;
		OrbSpots.Add(FVector2D(CursorY + FromCursor + Span * T, CursorZ + Height + Arc * FMath::Sin(PI * T)));
	}
}

void APathCourse::Enemy(EPathSpawn Kind, float Back, float Height, float PatrolBack, float PatrolFront)
{
	Spawns.Add({ Kind, CursorY - Back, CursorZ + Height, CursorY - PatrolBack, CursorY - PatrolFront });
}

void APathCourse::Checkpoint(float Back)
{
	Checkpoints.Add(FVector2D(CursorY - Back, CursorZ));
}

void APathCourse::BuildCourse()
{
	// Lightwalker runs at 640 cm/s. One jump clears about 250 up and 440 across; two clear about 420 up and 720 across.
	// Nothing below asks for more than 80% of that.

	// 1. Spikes (the first level of the 2014 Green Ball plan): one jump at a time, then gaps.
	Ground(1300.f);
	Checkpoint(1050.f);
	Orbs(-500.f, 3, 240.f, 90.f);
	Ground(700.f);
	SpikesBack(380.f, 70.f);
	Orbs(-345.f, 1, 0.f, 260.f);
	Ground(700.f);
	SpikesBack(420.f, 140.f);
	Orbs(-350.f, 1, 0.f, 290.f);
	Gap(230.f);
	Ground(740.f);
	SpikesBack(460.f, 70.f);
	Orbs(40.f, 3, 180.f, 130.f, 90.f);
	Gap(270.f);
	Rise(90.f);
	Ground(640.f);
	SpikesBack(390.f, 140.f);
	Gap(280.f);
	Rise(-150.f);
	Orbs(-150.f, 1, 0.f, 300.f);
	Ground(700.f);
	Checkpoint(350.f);

	// 2. Double jumps (the second level of the plan).
	Orbs(90.f, 4, 330.f, 190.f, 120.f);
	Gap(510.f);
	Ground(520.f);
	Rise(320.f);
	Orbs(-60.f, 1, 0.f, 80.f);
	Ground(700.f);
	SpikesBack(420.f, 140.f);
	AddBlock(EPathBlock::Ledge, 260.f, 240.f, 60.f);
	Orbs(380.f, 1, 0.f, 150.f);
	AddBlock(EPathBlock::Ledge, 800.f, 220.f, 180.f);
	Orbs(910.f, 1, 0.f, 270.f);
	Gap(1300.f);
	Rise(60.f);
	Ground(660.f);
	SpikesBack(420.f, 70.f);
	Gap(300.f);
	Rise(-320.f);
	Ground(420.f);
	// Carried across: the moving ledge.
	AddBlock(EPathBlock::Moving, 120.f, 260.f, 0.f, FVector2D(620.f, 0.f), 5.f);
	Orbs(560.f, 2, 220.f, 130.f);
	Gap(1120.f);
	Ground(760.f);
	SpikesBack(520.f, 200.f);
	Orbs(-420.f, 1, 0.f, 330.f);
	Checkpoint(170.f);

	// 3. The enemies 2014 never managed.
	Gap(260.f);
	Ground(1700.f);
	Enemy(EPathSpawn::Shaytaan, 1000.f, 0.f, 1350.f, 750.f);
	Enemy(EPathSpawn::Shaytaan, 450.f, 0.f, 650.f, 60.f);
	// The book of knowledge, off the main way: two ledges up.
	AddBlock(EPathBlock::Ledge, -1250.f, 220.f, 230.f);
	AddBlock(EPathBlock::Ledge, -880.f, 220.f, 440.f);
	Orbs(-1140.f, 1, 0.f, 320.f);
	Spawns.Add({ EPathSpawn::Book, CursorY - 770.f, CursorZ + 440.f });
	Orbs(-420.f, 2, 220.f, 110.f);
	Gap(280.f);
	Ground(900.f);
	Enemy(EPathSpawn::Dajjal, 320.f, 110.f);
	SpikesBack(640.f, 70.f);
	Orbs(-760.f, 1, 0.f, 100.f);
	// Crumbling ledges: keep moving.
	AddBlock(EPathBlock::Crumbling, 170.f, 230.f, 0.f);
	AddBlock(EPathBlock::Crumbling, 560.f, 230.f, 40.f);
	AddBlock(EPathBlock::Crumbling, 950.f, 230.f, 0.f);
	Orbs(285.f, 3, 780.f, 150.f);
	Gap(1350.f);
	Ground(760.f);
	Enemy(EPathSpawn::Shaytaan, 380.f, 0.f, 600.f, 250.f);
	Checkpoint(130.f);

	// 4. Everything together.
	AddBlock(EPathBlock::Moving, 90.f, 280.f, -20.f, FVector2D(0.f, 520.f), 5.f);
	Orbs(230.f, 2, 0.f, 330.f, 0.f);
	Gap(460.f);
	Rise(500.f);
	Ground(720.f);
	SpikesBack(430.f, 140.f);
	Orbs(80.f, 3, 340.f, 190.f, 120.f);
	Gap(500.f);
	Rise(-120.f);
	AddBlock(EPathBlock::Crumbling, 0.f, 260.f, 0.f);
	Gap(560.f);
	Ground(620.f);
	SpikesBack(420.f, 70.f);
	AddBlock(EPathBlock::Ledge, 420.f, 240.f, 40.f);
	Orbs(540.f, 1, 0.f, 140.f);
	AddBlock(EPathBlock::Crumbling, 960.f, 230.f, 60.f);
	Orbs(1075.f, 1, 0.f, 160.f);
	Gap(1400.f);
	Rise(-120.f);
	Ground(1500.f);
	Enemy(EPathSpawn::Shaytaan, 1050.f, 0.f, 1250.f, 800.f);
	Enemy(EPathSpawn::Dajjal, 680.f, 120.f);
	Enemy(EPathSpawn::Shaytaan, 380.f, 0.f, 620.f, 60.f);
	Orbs(-300.f, 2, 180.f, 110.f);
	Gap(300.f);
	Ground(1800.f);
	SpikesBack(1500.f, 140.f);
	Orbs(-1100.f, 5, 480.f, 110.f);
	GateY = CursorY - 620.f;
}

// ---------------------------------------------------------------------------------------------------------------
// Meshes

void APathCourse::BeginPlay()
{
	Super::BeginPlay();
	BuildMeshes();
	BuildBackdrop();
}

void APathCourse::BuildMeshes()
{
	for (FPathBlock& Block : Blocks)
	{
		const float Width = Block.Right - Block.Left;
		const float Middle = (Block.Left + Block.Right) * 0.5f;
		const float Bottom = Block.Kind == EPathBlock::Ground ? AbyssZ : Block.Top - LedgeThickness;

		FPart Part;
		Part.BodyStart = FVector(0.f, Middle, (Block.Top + Bottom) * 0.5f);
		UStaticMeshComponent* Body = LWNeon::AddMesh(this, ELWShape::Cube, RootComponent, Part.BodyStart, FVector(PathDepth * 2.f, Width, Block.Top - Bottom) / 100.f);
		LWNeon::ApplyRock(Body, GroundColor);

		// The glowing lip along the front edge.
		Part.EdgeStart = FVector(-PathDepth - 2.f, Middle, Block.Top - 5.f);
		UStaticMeshComponent* Edge = LWNeon::AddMesh(this, ELWShape::Cube, RootComponent, Part.EdgeStart, FVector(0.05f, Width / 100.f, 0.1f));
		Part.Body = Body;
		Part.Edge = Edge;
		Part.EdgeMaterial = LWNeon::ApplyGlow(Edge, EdgeColorFor(Block.Kind), EdgeGlowInDark);
		Block.Part = Parts.Add(Part);
	}

	// The camera barely tilts, so upright cards facing down the course read as solid.
	const FRotator Facing = LWNeon::CardFacing(FRotator::ZeroRotator);
	auto AddArt = [&](const TCHAR* Name, const FVector& Location, const FVector2D& Size, int32 SortPriority = 0) -> UMaterialInstanceDynamic*
	{
		UStaticMeshComponent* Card = LWNeon::AddCard(this, LWStyle::Art(Name), RootComponent, Location, Size, Facing);
		if (!Card)
		{
			return nullptr;
		}
		Card->SetTranslucentSortPriority(SortPriority);
		return Cast<UMaterialInstanceDynamic>(Card->GetMaterial(0));
	};
	auto AddLight = [this](const FVector& Location, float Intensity, float Radius, const FLinearColor& Colour)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(RootComponent);
		Light->SetRelativeLocation(Location);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(Intensity);
		Light->SetAttenuationRadius(Radius);
		Light->SetLightColor(Colour);
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		return Light;
	};

	// Spikes: clusters of crystal in three rows, the middle one mirrored so they don't line up.
	constexpr float SpikeHeight = 118.f;
	constexpr float SpikeAspect = 1024.f / 616.f;
	for (const FPathSpikes& Row : Spikes)
	{
		FSpikeRow& Shown = SpikeRows.AddDefaulted_GetRef();
		Shown.Centre = (Row.Left + Row.Right) * 0.5f;
		const int32 Count = FMath::Max(1, FMath::RoundToInt((Row.Right - Row.Left) / (SpikeHeight * SpikeAspect * 0.85f)));
		const float Step = (Row.Right - Row.Left) / Count;
		for (const float Depth : { 60.f, 0.f, -60.f })
		{
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const float Height = SpikeHeight * (Depth == 0.f ? 0.82f : 1.f);
				Shown.Materials.Add(AddArt(TEXT("Spikes"), FVector(Depth, Row.Left + Step * (Index + 0.5f), Row.Base + Height * 0.5f - 4.f), FVector2D(Step * 1.08f * (Depth == 0.f ? -1.f : 1.f), Height)));
			}
		}
	}

	AddTrimmings(Facing);

	// Checkpoint lanterns: a post at the back of the path, lit as you pass.
	const FVector2D PostSize = FVector2D(392.f, 1000.f) * 0.3f;
	const FVector2D Lamp(0.25f, 0.25f);  // where the lantern is on the art, from its middle
	for (const FVector2D& Spot : Checkpoints)
	{
		const FVector Middle(70.f, Spot.X, Spot.Y + PostSize.Y * 0.5f - 4.f);
		FLantern& Lantern = Lanterns.AddDefaulted_GetRef();
		Lantern.Y = Spot.X;
		Lantern.Post = AddArt(TEXT("LanternUnlit"), Middle, PostSize);
		Lantern.Lit = AddArt(TEXT("LanternLit"), Middle - FVector(1.f, 0.f, 0.f), PostSize, 1);
		Lantern.Light = AddLight(Middle + FVector(-60.f, PostSize.X * Lamp.X, PostSize.Y * Lamp.Y), 0.f, 650.f, FLinearColor(1.f, 0.75f, 0.4f));
	}

	// The gate of light, facing the camera from the back of the path.
	float GateZ = 0.f;
	for (const FPathBlock& Block : Blocks)
	{
		if (Block.Kind == EPathBlock::Ground && Block.Left <= GateY && Block.Right >= GateY)
		{
			GateZ = Block.Top;
		}
	}
	const FVector2D GateSize = FVector2D(1024.f, 1536.f) * 0.29f;
	GateMaterial = AddArt(TEXT("PathGate"), FVector(95.f, GateY, GateZ + GateSize.Y * 0.5f - 6.f), GateSize);
	AddLight(FVector(-60.f, GateY, GateZ + 240.f), 90.f, 1500.f, GateColor);
}

void APathCourse::AddTrimmings(const FRotator& Facing)
{
	// The season's icicles, blossom, leaves or string of lanterns, hung along the front lip of the ground.
	UTexture2D* Hanging = LWSeason::OwnArt(TEXT("PathTrim"));
	if (!Hanging)
	{
		return;
	}
	const FIntPoint Art = Hanging->GetImportedSize();
	const float Aspect = static_cast<float>(Art.X) / FMath::Max(1, Art.Y);
	// Lanterns need more room than a fringe.
	const ELWSeason Season = LWSeason::Current();
	const float Height = Season == ELWSeason::Ramadan || Season == ELWSeason::Eid ? 130.f : 74.f;
	for (const FPathBlock& Block : Blocks)
	{
		const float Width = Block.Right - Block.Left;
		if (Block.Kind != EPathBlock::Ground || Width < Height * Aspect * 0.6f)
		{
			continue;
		}
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Width / (Height * Aspect)));
		const float Step = Width / Count;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Where(-PathDepth - 4.f, Block.Left + Step * (Index + 0.5f), Block.Top - 10.f - Height * 0.5f);
			if (UStaticMeshComponent* Card = LWNeon::AddCard(this, Hanging, RootComponent, Where, FVector2D(Step, Height), Facing))
			{
				Trimmings.Add({ Cast<UMaterialInstanceDynamic>(Card->GetMaterial(0)), static_cast<float>(Where.Y) });
			}
		}
	}
}

void APathCourse::BuildBackdrop()
{
	// Pictures behind the course, each repeating along it. They're see-through, so they're drawn in this order whatever
	// the distances.
	const FRotator Facing = LWNeon::CardFacing(FRotator::ZeroRotator);
	auto AddLayer = [&](const TCHAR* Name, USceneComponent* Parent, float X, float Bottom, const FVector2D& Size, float From, float To, float Intensity, int32 Order)
	{
		for (float Y = From; Y < To; Y += Size.X)
		{
			if (UStaticMeshComponent* Card = LWNeon::AddCard(this, LWSeason::Art(Name), Parent, FVector(X, Y + Size.X * 0.5f, Bottom + Size.Y * 0.5f), Size, Facing, Intensity))
			{
				Card->SetTranslucentSortPriority(Order);
			}
		}
	};

	// The sky travels with the camera, with plain dark behind it in case a jump sees past its top.
	UStaticMeshComponent* Night = LWNeon::AddMesh(this, ELWShape::Cube, Sky, FVector(17500.f, 0.f, 0.f), FVector(1.f, 1600.f, 500.f));
	LWNeon::ApplyGlow(Night, FLinearColor(0.004f, 0.001f, 0.012f), 1.f);
	const FVector2D SkySize = FVector2D(1968.f, 724.f) * 9.6f;
	AddLayer(TEXT("PathSky"), Sky, 16000.f, -300.f, SkySize, -SkySize.X * 2.5f, SkySize.X * 2.4f, 1.f, -50);

	// The sun, rising as the gate gets nearer.
	// (In Ramadan and at Eid, a crescent moon.)
	UTexture2D* Moon = LWSeason::OwnArt(TEXT("PathSun"));
	if (UStaticMeshComponent* Disc = LWNeon::AddCard(this, Moon ? Moon : LWStyle::Art(TEXT("PathSun")), Sun, FVector::ZeroVector, FVector2D(Moon ? 4600.f : 7600.f), Facing, Moon ? 1.4f : 1.1f))
	{
		Disc->SetTranslucentSortPriority(-40);
	}

	// Far mountains and nearer ridges, which the camera passes at their own pace. Wide enough for a 32:9 screen at either end.
	// The mountains' foot is see-through, so the sun needs something to set behind.
	UStaticMeshComponent* Foot = LWNeon::AddMesh(this, ELWShape::Cube, RootComponent, FVector(10000.f, GateY * 0.5f, -1650.f), FVector(1.f, (GateY + 48000.f) / 100.f, 15.f));
	LWNeon::ApplyGlow(Foot, FLinearColor(0.045f, 0.004f, 0.04f), 1.f);
	AddLayer(TEXT("PathMountains"), RootComponent, 9800.f, -1750.f, FVector2D(2044.f, 428.f) * 8.4f, -24000.f, GateY + 24000.f, 1.f, -30);
	AddLayer(TEXT("PathRidges"), RootComponent, 4900.f, -1800.f, FVector2D(2044.f, 424.f) * 4.6f, -16000.f, GateY + 16000.f, 0.65f, -20);

	// The abyss: a dark floor with the arena's grid, far below.
	UStaticMeshComponent* Floor = LWNeon::AddMesh(this, ELWShape::Cube, RootComponent, FVector(4000.f, GateY * 0.5f, AbyssZ - 60.f), FVector(160.f, (GateY + 40000.f) / 100.f, 1.f));
	LWNeon::ApplyGlow(Floor, FLinearColor(0.008f, 0.002f, 0.018f), 1.f);
	UInstancedStaticMeshComponent* Grid = LWNeon::AddInstances(this, ELWShape::Cube, RootComponent);
	for (float Y = -14000.f; Y < GateY + 16000.f; Y += 500.f)
	{
		Grid->AddInstance(FTransform(FRotator::ZeroRotator, FVector(4000.f, Y, AbyssZ), FVector(140.f, 0.09f, 0.02f)));
	}
	for (float X = -1500.f; X < 9000.f; X += 500.f)
	{
		Grid->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, GateY * 0.5f, AbyssZ), FVector(0.09f, (GateY + 30000.f) / 100.f, 0.02f)));
	}
	LWNeon::ApplyGlow(Grid, FLinearColor(0.55f, 0.12f, 1.f), 0.3f);
}

// ---------------------------------------------------------------------------------------------------------------
// Running

float APathCourse::GetLowestNear(float Y) const
{
	float Lowest = TNumericLimits<float>::Max();
	for (const FPathBlock& Block : Blocks)
	{
		if (Block.GetRight() > Y - 1600.f && Block.GetLeft() < Y + 1600.f)
		{
			Lowest = FMath::Min(Lowest, Block.Top + FMath::Min(0.f, static_cast<float>(Block.Travel.Y)));
		}
	}
	return Lowest == TNumericLimits<float>::Max() ? 0.f : Lowest;
}

bool APathCourse::IsSolidAt(float Y, float Z) const
{
	for (const FPathBlock& Block : Blocks)
	{
		if (Y > Block.GetLeft() && Y < Block.GetRight() && Z < Block.GetTop())
		{
			if (Block.Kind == EPathBlock::Ground || (Block.IsSolid() && Z > Block.GetTop() - LedgeThickness))
			{
				return true;
			}
		}
	}
	return false;
}

void APathCourse::LightCheckpoint(int32 Index)
{
	if (Lanterns.IsValidIndex(Index))
	{
		Lanterns[Index].bLit = true;
	}
}

void APathCourse::StandOn(int32 BlockIndex)
{
	if (Blocks.IsValidIndex(BlockIndex) && Blocks[BlockIndex].Kind == EPathBlock::Crumbling && Blocks[BlockIndex].CrumbleTime < 0.f)
	{
		Blocks[BlockIndex].CrumbleTime = 0.f;
	}
}

void APathCourse::ResetCrumbling()
{
	for (FPathBlock& Block : Blocks)
	{
		Block.CrumbleTime = -1.f;
	}
}

void APathCourse::UpdateBlocks(float DeltaSeconds)
{
	CourseTime += DeltaSeconds;
	// How well a spot along the course shows in Lightwalker's light.
	auto SeenAt = [this](float Y) { return 1.f - FMath::SmoothStep(Reach * 0.55f, Reach * 1.5f, static_cast<float>(FMath::Abs(Target.Y - Y))); };
	if (UMaterialInstanceDynamic* Gate = GateMaterial.Get())
	{
		Gate->SetScalarParameterValue(LWNeon::IntensityParam, 1.5f + 0.15f * FMath::Sin(CourseTime * 1.7f));
	}
	for (FPathBlock& Block : Blocks)
	{
		const FVector2D Before = Block.Offset;
		FVector Shake = FVector::ZeroVector;
		float Dim = 1.f;
		if (Block.Kind == EPathBlock::Moving)
		{
			// Eases to a stop at each end.
			Block.Offset = Block.Travel * (0.5f - 0.5f * FMath::Cos(CourseTime * 2.f * PI / Block.Period));
		}
		else if (Block.Kind == EPathBlock::Crumbling && Block.CrumbleTime >= 0.f)
		{
			Block.CrumbleTime += DeltaSeconds;
			if (Block.CrumbleTime < 0.55f)
			{
				Shake = FVector(0.f, FMath::Sin(Block.CrumbleTime * 70.f) * 4.f, FMath::Cos(Block.CrumbleTime * 55.f) * 3.f);
			}
			else if (Block.CrumbleTime < 3.2f)
			{
				const float Fall = Block.CrumbleTime - 0.55f;
				Shake = FVector(0.f, 0.f, -1400.f * Fall * Fall);
				Dim = FMath::Max(0.f, 1.f - Fall * 2.f);
			}
			else if (Block.CrumbleTime > 3.5f)
			{
				Block.CrumbleTime = -1.f;
			}
		}
		Block.FrameMove = Block.Offset - Before;

		FPart& Part = Parts[Block.Part];
		if (Block.Kind != EPathBlock::Ground)
		{
			const FVector Move = FVector(0.f, Block.Offset.X, Block.Offset.Y) + Shake;
			Part.Body->SetRelativeLocation(Part.BodyStart + Move);
			Part.Edge->SetRelativeLocation(Part.EdgeStart + Move);
		}

		// The path shows itself only where the light reaches.
		const float Y = FMath::Clamp(Target.Y, Block.GetLeft(), Block.GetRight());
		const float Distance = FMath::Sqrt(FMath::Square(Target.Y - Y) + FMath::Square(0.6f * (Target.Z - Block.GetTop())));
		const float Seen = 1.f - FMath::SmoothStep(Reach * 0.55f, Reach * 1.5f, static_cast<float>(Distance));
		if (UMaterialInstanceDynamic* Material = Part.EdgeMaterial.Get())
		{
			Material->SetScalarParameterValue(LWNeon::IntensityParam, FMath::Lerp(EdgeGlowInDark, EdgeGlow, Seen) * Dim);
		}
	}

	for (FSpikeRow& Row : SpikeRows)
	{
		for (const TWeakObjectPtr<UMaterialInstanceDynamic>& Material : Row.Materials)
		{
			if (Material.IsValid())
			{
				Material->SetScalarParameterValue(LWNeon::IntensityParam, FMath::Lerp(0.05f, 1.4f, SeenAt(Row.Centre)));
			}
		}
	}

	for (const FTrimming& Trimming : Trimmings)
	{
		// They show only where the light reaches, like the edges, or they'd give the dark away.
		if (UMaterialInstanceDynamic* Material = Trimming.Material.Get())
		{
			Material->SetScalarParameterValue(LWNeon::IntensityParam, FMath::Lerp(0.04f, 1.1f, SeenAt(Trimming.Y)));
		}
	}

	for (FLantern& Lantern : Lanterns)
	{
		Lantern.Blend = FMath::FInterpTo(Lantern.Blend, Lantern.bLit ? 1.f : 0.f, DeltaSeconds, 3.f);
		const float Flicker = 1.f + 0.08f * FMath::Sin(CourseTime * 11.f) + 0.05f * FMath::Sin(CourseTime * 17.3f);
		if (UMaterialInstanceDynamic* Post = Lantern.Post.Get())
		{
			Post->SetScalarParameterValue(LWNeon::IntensityParam, FMath::Lerp(0.06f, 0.8f, SeenAt(Lantern.Y)));
		}
		if (UMaterialInstanceDynamic* Lit = Lantern.Lit.Get())
		{
			Lit->SetScalarParameterValue(LWNeon::IntensityParam, 1.5f * Flicker);
			Lit->SetScalarParameterValue(LWNeon::OpacityParam, Lantern.Blend);
		}
		if (UPointLightComponent* Light = Lantern.Light.Get())
		{
			Light->SetIntensity(26.f * Lantern.Blend * Flicker);
		}
	}
}

void APathCourse::Follow(const FVector& Location, float Facing, float LightReach, bool bInstant)
{
	Target = Location;
	TargetFacing = Facing;
	Reach = LightReach;
	if (bInstant)
	{
		UpdateCamera(0.f, true);
	}
}

void APathCourse::UpdateCamera(float DeltaSeconds, bool bInstant)
{
	FacingBlend = bInstant ? TargetFacing : FMath::FInterpTo(FacingBlend, TargetFacing, DeltaSeconds, 2.2f);
	const FVector Wanted(-CameraDistance, Target.Y + LookAhead * FacingBlend, Target.Z + LookUp);
	if (bInstant)
	{
		CameraAt = Wanted;
	}
	else
	{
		CameraAt.X = Wanted.X;
		CameraAt.Y = FMath::FInterpTo(CameraAt.Y, Wanted.Y, DeltaSeconds, 9.f);
		CameraAt.Z = FMath::FInterpTo(CameraAt.Z, Wanted.Z, DeltaSeconds, 4.f);
	}

	const float Shake = Trauma * Trauma;
	const FVector ShakeMove = FVector(0.f, FMath::PerlinNoise1D(ShakeTime * 23.f), FMath::PerlinNoise1D(ShakeTime * 23.f + 50.f)) * 22.f * Shake;
	Camera->SetWorldLocation(CameraAt + ShakeMove);
	Camera->SetWorldRotation(FRotator(CameraPitch, 0.f, FMath::PerlinNoise1D(ShakeTime * 19.f + 150.f) * 1.1f * Shake));

	Camera->SetFieldOfView(LWNeon::FitFieldOfView(CameraFieldOfView, LWNeon::GetViewAspect(GetWorld())));

	// The sky travels with the camera; the sun climbs with progress.
	const float Progress = FMath::Clamp(Target.Y / FMath::Max(GateY, 1.f), 0.f, 1.f);
	Sky->SetWorldLocation(FVector(0.f, CameraAt.Y, CameraAt.Z * 0.8f));
	Sun->SetRelativeLocation(FVector(15000.f, 2600.f, FMath::Lerp(-1700.f, 1700.f, FMath::Pow(Progress, 1.3f))));
}

void APathCourse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Trauma > 0.f)
	{
		ShakeTime += DeltaSeconds;
		Trauma = FMath::Max(0.f, Trauma - 1.6f * DeltaSeconds);
	}
	UpdateBlocks(DeltaSeconds);
	UpdateCamera(DeltaSeconds, false);

	// The dark closes in from the edges and the colour drains away as the light goes.
	ShownLight = FMath::FInterpTo(ShownLight, LightLevel, DeltaSeconds, 4.f);
	const float Dark = FMath::Square(1.f - ShownLight);
	FPostProcessSettings& Settings = PostProcess->Settings;
	Settings.VignetteIntensity = FMath::Lerp(0.5f, 1.7f, Dark);
	Settings.AutoExposureBias = FMath::Lerp(0.f, -2.2f, Dark);
}
