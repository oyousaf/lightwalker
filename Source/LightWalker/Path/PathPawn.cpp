#include "Path/PathPawn.h"

#include "Components/PointLightComponent.h"
#include "Core/LightWalkerGameInstance.h"
#include "Core/LWSound.h"
#include "Effects/LWParticles.h"
#include "Enemies/EnemyBase.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "LightWalker.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Path/PathCourse.h"

namespace
{
	/** Lightwalker from the feet up: what enemies can touch. */
	constexpr float BodyHeight = 120.f;
	/** Falling this far below the lowest ground nearby is gone. */
	constexpr float FallDepth = 700.f;
}

APathPawn::APathPawn()
{
	Heading = FVector::RightVector;
}

float APathPawn::GetFeet() const
{
	// The hero's feet are a contact radius below the actor (see ALightWalkerPawn::UseModel).
	return GetActorLocation().Z - ContactRadius;
}

void APathPawn::SetCourse(APathCourse* InCourse)
{
	Course = InCourse;
	if (Course)
	{
		// The course moves its ledges first, so anyone standing on one is carried the same frame.
		AddTickPrerequisiteActor(Course);
	}
}

void APathPawn::PlaceAt(const FVector2D& Spot, float GraceSeconds)
{
	Revive(FVector(0.f, Spot.X, 0.f), GraceSeconds);
	SetActorLocation(FVector(0.f, Spot.X, Spot.Y + ContactRadius));
	Velocity = FVector2D::ZeroVector;
	bGrounded = true;
	JumpsUsed = 0;
	SinceGround = 0.f;
	SincePress = 1000.f;
	StandingOn = INDEX_NONE;
	Hazard = EPathHazard::None;
	Facing = 1.f;
	Heading = FVector::RightVector;
	FocusZ = GetActorLocation().Z;
	AirBlend = 0.f;
}

void APathPawn::SetJumpHeld(bool bPressed)
{
	if (bPressed && !bJumpHeld)
	{
		SincePress = 0.f;
	}
	bJumpHeld = bPressed;
}

bool APathPawn::ProjectCursor(const FVector& Origin, const FVector& Direction, FVector& OutPoint) const
{
	// The plane of the path.
	const float Distance = FMath::IsNearlyZero(Direction.X) ? -1.f : (GetActorLocation().X - Origin.X) / Direction.X;
	OutPoint = Origin + Direction * Distance;
	return Distance > 0.f;
}

bool APathPawn::IsWithin(const FVector& Point, float Distance) const
{
	// Lightwalker stands tall here, so measure to the nearest point of the body rather than its centre.
	const FVector Here = GetActorLocation();
	const float Z = FMath::Clamp(Point.Z, Here.Z, Here.Z + BodyHeight - ContactRadius);
	return FMath::Square(Point.Y - Here.Y) + FMath::Square(Point.Z - Z) <= FMath::Square(Distance);
}

float APathPawn::GetLightReach() const
{
	return Glow->AttenuationRadius;
}

FVector APathPawn::GetCameraFocus() const
{
	const FVector Here = GetActorLocation();
	return FVector(0.f, Here.Y, FocusZ);
}

FVector APathPawn::GetCursorDirection() const
{
	const FVector To = CursorTarget - (GetActorLocation() + FVector(0.f, 0.f, 25.f));
	return FVector(0.f, To.Y, To.Z).GetSafeNormal();
}

FVector APathPawn::GetMuzzle(const FVector& Direction) const
{
	return GetActorLocation() + FVector(0.f, Direction.Y * 50.f, 25.f + Direction.Z * 50.f);
}

FBox2D APathPawn::GetShotBounds() const
{
	const float Y = GetActorLocation().Y;
	return FBox2D(FVector2D(-500.f, Y - 1500.f), FVector2D(500.f, Y + 1500.f));
}

void APathPawn::FaceDirection(const FVector& Direction)
{
	// Faces along the path, turned a little towards the camera, and swings round rather than snapping.
	if (FMath::Abs(Direction.Y) > 0.1f)
	{
		Facing = FMath::Sign(Direction.Y);
	}
	const float Wanted = Facing > 0.f ? 108.f : 252.f;  // yaw 90 is screen right, 180 faces the camera
	FacingYaw = FMath::FInterpTo(FacingYaw, Wanted, GetWorld()->GetDeltaSeconds(), 16.f);
	HeadingPivot->SetWorldRotation(FRotator(0.f, FacingYaw, 0.f));
}

void APathPawn::Jump(bool bSecond)
{
	Velocity.Y = bSecond ? SecondJumpSpeed : JumpSpeed;
	bGrounded = false;
	StandingOn = INDEX_NONE;
	SincePress = 1000.f;
	SinceGround = 1000.f;
	JumpsUsed = bSecond ? 2 : 1;
	bJumpsCut = false;
	ULWSoundSubsystem::Play(this, bSecond ? ELWSound::DoubleJump : ELWSound::Jump);

	// The second jump pushes off a burst of light.
	FLWEmit Puff;
	Puff.Count = bSecond ? 16 : 6;
	Puff.Origin = GetActorLocation() - FVector(0.f, 0.f, ContactRadius);
	Puff.Direction = -FVector::UpVector;
	Puff.Spread = bSecond ? 80.f : 85.f;
	Puff.Speed = bSecond ? FVector2D(180.f, 420.f) : FVector2D(80.f, 220.f);
	Puff.Life = FVector2D(0.2f, 0.45f);
	Puff.Size = FVector2D(0.035f, bSecond ? 0.08f : 0.05f);
	Puff.Drag = 4.f;
	LWFX::Burst(GetWorld(), ELWShape::Sphere, FLinearColor(1.f, 0.9f, 0.6f), bSecond ? 11.f : 5.f, { Puff });
}

void APathPawn::Land(int32 BlockIndex)
{
	if (!bGrounded && Velocity.Y < -700.f)
	{
		ULWSoundSubsystem::Play(this, ELWSound::Land);
		FLWEmit Dust;
		Dust.Count = 8;
		Dust.Origin = GetActorLocation() - FVector(0.f, 0.f, ContactRadius - 4.f);
		Dust.Direction = FVector::UpVector;
		Dust.Spread = 85.f;
		Dust.Speed = FVector2D(90.f, 240.f);
		Dust.Life = FVector2D(0.2f, 0.4f);
		Dust.Size = FVector2D(0.025f, 0.05f);
		Dust.Drag = 5.f;
		LWFX::Burst(GetWorld(), ELWShape::Sphere, FLinearColor(0.8f, 0.75f, 1.f), 3.f, { Dust });
	}
	bGrounded = true;
	JumpsUsed = 0;
	SinceGround = 0.f;
	Velocity.Y = 0.f;
	StandingOn = BlockIndex;
	Course->StandOn(BlockIndex);
}

void APathPawn::Step(float MoveY, float DeltaSeconds)
{
	const TArray<FPathBlock>& Blocks = Course->GetBlocks();
	FVector Location = GetActorLocation();

	// Along the path.
	const float Speed = RunSpeed * (GetRun() ? GetRun()->GetUpgrades().MoveSpeed : 1.f);
	Velocity.X = FMath::FInterpConstantTo(Velocity.X, MoveY * Speed, DeltaSeconds, bGrounded ? GroundAcceleration : AirAcceleration);
	Location.Y += Velocity.X * DeltaSeconds;

	float Feet = Location.Z - ContactRadius;
	for (const FPathBlock& Block : Blocks)
	{
		// Only solid ground has sides. A lip this small is stepped onto instead.
		if (Block.Kind == EPathBlock::Ground && Feet < Block.Top - 14.f &&
			Location.Y + HalfWidth > Block.Left && Location.Y - HalfWidth < Block.Right)
		{
			const bool bFromLeft = GetActorLocation().Y < (Block.Left + Block.Right) * 0.5f;
			Location.Y = bFromLeft ? Block.Left - HalfWidth : Block.Right + HalfWidth;
			Velocity.X = 0.f;
		}
	}
	Location.Y = FMath::Max(Location.Y, (Blocks.Num() > 0 ? Blocks[0].Left : 0.f) + HalfWidth);

	// Up and down: rise slower than falling, and cut the rise short when jump is let go.
	float Pull = Gravity;
	if (Velocity.Y <= 0.f)
	{
		Pull *= FallGravityScale;
	}
	else if (bJumpsCut)
	{
		Pull *= CutGravityScale;
	}
	const float FeetBefore = Feet;
	Velocity.Y = FMath::Max(Velocity.Y - Pull * DeltaSeconds, -MaxFallSpeed);
	Location.Z += Velocity.Y * DeltaSeconds;
	Feet = Location.Z - ContactRadius;

	int32 LandedOn = INDEX_NONE;
	if (Velocity.Y <= 0.f)
	{
		float Best = -TNumericLimits<float>::Max();
		for (int32 Index = 0; Index < Blocks.Num(); ++Index)
		{
			const FPathBlock& Block = Blocks[Index];
			const float Top = Block.GetTop();
			const bool bOver = Location.Y + HalfWidth * 0.7f > Block.GetLeft() && Location.Y - HalfWidth * 0.7f < Block.GetRight();
			// It was above (allowing for the ledge's own movement and the step) and is now at or below the top.
			const float Allowance = 15.f + FMath::Abs(Block.FrameMove.Y);
			if (bOver && Block.IsSolid() && FeetBefore >= Top - Allowance && Feet <= Top && Top > Best)
			{
				Best = Top;
				LandedOn = Index;
			}
		}
		if (LandedOn != INDEX_NONE)
		{
			Location.Z = Best + ContactRadius;
		}
	}
	SetActorLocation(Location);

	if (LandedOn != INDEX_NONE)
	{
		Land(LandedOn);
	}
	else
	{
		bGrounded = false;
		StandingOn = INDEX_NONE;
		SinceGround += DeltaSeconds;
	}
}

FVector APathPawn::UpdateMovement(const FVector2D& Input, float DeltaSeconds)
{
	if (!Course)
	{
		return FVector::ZeroVector;
	}

	FVector2D Wanted = bHeld ? FVector2D::ZeroVector : Input;
#if !UE_BUILD_SHIPPING
	static const bool bAutoPath = FParse::Param(FCommandLine::Get(), TEXT("LWAutoPath"));
	if (bAutoPath && !bHeld)
	{
		Wanted = AutoPlay(DeltaSeconds);
	}
#endif

	// Carried by a moving ledge.
	const TArray<FPathBlock>& Blocks = Course->GetBlocks();
	if (bGrounded && Blocks.IsValidIndex(StandingOn))
	{
		const FVector2D Carry = Blocks[StandingOn].FrameMove;
		SetActorLocation(GetActorLocation() + FVector(0.f, Carry.X, Carry.Y));
	}

	// Jumping: from the ground (or just after it), then once more in the air.
	SincePress += DeltaSeconds;
	if (!bHeld && SincePress <= PressMemory)
	{
		if (bGrounded || (JumpsUsed == 0 && SinceGround <= EdgeForgiveness))
		{
			Jump(false);
		}
		else if (JumpsUsed < 2)
		{
			Jump(true);
		}
	}
	bJumpsCut |= !bJumpHeld && Velocity.Y > 0.f;

	// A hitch slows the world rather than throwing Lightwalker across it.
	DeltaSeconds = FMath::Min(DeltaSeconds, 1.f / 15.f);

	// Small fixed steps, so a slow frame can't pass through a ledge or jump a different distance.
	constexpr float MaxStep = 1.f / 120.f;
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(DeltaSeconds / MaxStep));
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		Step(Wanted.X, DeltaSeconds / Steps);
	}

	// The camera follows the ground, catching up with the air only when Lightwalker leaves its view.
	const float Z = GetActorLocation().Z;
	FocusZ = bGrounded ? FMath::FInterpTo(FocusZ, Z, DeltaSeconds, 6.f) : FMath::Clamp(FocusZ, Z - 110.f, Z + 40.f);

	AirBlend = FMath::FInterpTo(AirBlend, bGrounded ? 0.f : 1.f, DeltaSeconds, bGrounded ? 16.f : 12.f);
	AirRise = FMath::FInterpTo(AirRise, FMath::Clamp(Velocity.Y / 600.f, -1.f, 1.f), DeltaSeconds, 10.f);

	CheckHazards();

	const bool bRunning = FMath::Abs(Velocity.X) > 60.f && !FMath::IsNearlyZero(Wanted.X, 0.1f);
	return bRunning ? FVector(0.f, FMath::Sign(Wanted.X), 0.f) : FVector::ZeroVector;
}

void APathPawn::CheckHazards()
{
	const FVector Here = GetActorLocation();
	const float Feet = GetFeet();
	if (Feet < Course->GetLowestNear(Here.Y) - FallDepth)
	{
		Hazard = EPathHazard::Fell;
		return;
	}
	if (!IsVulnerable())
	{
		return;
	}
	for (const FPathSpikes& Row : Course->GetSpikes())
	{
		if (Here.Y + SpikeHalfWidth > Row.Left + 8.f && Here.Y - SpikeHalfWidth < Row.Right - 8.f && Feet < Row.Base + 58.f && Feet > Row.Base - 60.f)
		{
			Hazard = EPathHazard::Spikes;
			return;
		}
	}
}

#if !UE_BUILD_SHIPPING
FVector2D APathPawn::AutoPlay(float DeltaSeconds)
{
	const TArray<FPathBlock>& Blocks = Course->GetBlocks();
	const FVector Here = GetActorLocation();
	const float Feet = GetFeet();
	AutoJumpRelease -= DeltaSeconds;
	AutoTrace += DeltaSeconds;
	if (AutoTrace > 1.f)
	{
		AutoTrace = 0.f;
		UE_LOG(LogLightWalker, Verbose, TEXT("[Bot] Y %.0f feet %.0f vel %.0f %.0f ground %d on %d jumps %d"), Here.Y, Feet, Velocity.X, Velocity.Y, bGrounded, StandingOn, JumpsUsed);
	}
	if (AutoJumpRelease <= 0.f)
	{
		SetJumpHeld(false);
	}
	auto Press = [this](float Hold)
	{
		SetJumpHeld(false);
		SetJumpHeld(true);
		AutoJumpRelease = Hold;
	};
	auto GroundUnder = [&](float Y, float MaxTop) -> const FPathBlock*
	{
		const FPathBlock* Best = nullptr;
		for (const FPathBlock& Block : Blocks)
		{
			if (Block.IsSolid() && Y > Block.GetLeft() && Y < Block.GetRight() && Block.GetTop() <= MaxTop && (!Best || Block.GetTop() > Best->GetTop()))
			{
				Best = &Block;
			}
		}
		return Best;
	};

	// Shoot what's ahead, and hold back until it's gone.
	bFireHeld = false;
	float Move = 1.f;
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		const FVector To = It->GetActorLocation() - Here;
		if (To.Y > -60.f && To.Y < 900.f && FMath::Abs(To.Z) < 200.f)
		{
			AimInput = FVector2D(To.Y, To.Z - 25.f).GetSafeNormal();
			bFireHeld = true;
			Move = To.Y < 520.f ? (To.Y < 300.f ? -0.6f : 0.f) : Move;
		}
	}
	if (!bFireHeld)
	{
		AimInput = FVector2D::ZeroVector;
	}

	if (!bGrounded && AutoTarget == INDEX_NONE && !GroundUnder(Here.Y - 10.f, Feet + 10.f))
	{
		// Off an edge without meaning to be: make for whatever comes next.
		for (int32 Index = 0; Index < Blocks.Num(); ++Index)
		{
			if (Blocks[Index].IsSolid() && Blocks[Index].GetLeft() > Here.Y && (AutoTarget == INDEX_NONE || Blocks[Index].GetLeft() < Blocks[AutoTarget].GetLeft()))
			{
				AutoTarget = Index;
			}
		}
	}

	if (bGrounded)
	{
		AutoTarget = INDEX_NONE;
		const FPathBlock* Wall = nullptr;
		for (const FPathBlock& Block : Blocks)
		{
			if (Block.Kind == EPathBlock::Ground && Block.Left > Here.Y && Block.Left < Here.Y + 190.f && Block.Top > Feet + 20.f)
			{
				Wall = &Block;
			}
		}
		const bool bEdge = !GroundUnder(Here.Y + 45.f, Feet + 15.f) || GroundUnder(Here.Y + 45.f, Feet + 15.f)->GetTop() < Feet - 60.f;
		bool bSpikes = false;
		for (const FPathSpikes& Row : Course->GetSpikes())
		{
			bSpikes |= Row.Left > Here.Y && Row.Left < Here.Y + 85.f && FMath::Abs(Row.Base - Feet) < 40.f;
		}

		// Riding something slow: wait for the far side to come within reach.
		const FPathBlock* Next = nullptr;
		for (const FPathBlock& Block : Blocks)
		{
			if (Block.IsSolid() && Block.GetLeft() > Here.Y + 10.f && (!Next || Block.GetLeft() < Next->GetLeft()))
			{
				Next = &Block;
			}
		}
		const float Reach = Next ? Next->GetLeft() - Here.Y : 0.f;
		const float Climb = Next ? Next->GetTop() - Feet : 0.f;
		const bool bInReach = Reach < 600.f && Climb < 370.f;
		if (Wall && Wall->Top - Feet > 370.f)
		{
			const FPathBlock* Under = Blocks.IsValidIndex(StandingOn) ? &Blocks[StandingOn] : nullptr;
			const float Middle = Under ? (Under->GetLeft() + Under->GetRight()) * 0.5f : Here.Y;
			return FVector2D(FMath::Clamp((Middle - Here.Y) / 40.f, -1.f, 1.f), 0.f);
		}
		if ((bEdge || bSpikes || Wall) && Move > 0.f)
		{
			if (bEdge && !bInReach)
			{
				return FVector2D::ZeroVector;
			}
			AutoTarget = (bEdge || Wall) && Next ? static_cast<int32>(Next - Blocks.GetData()) : INDEX_NONE;
			Press(0.4f);
		}
	}
	else if (Blocks.IsValidIndex(AutoTarget))
	{
		// Steer for the block being jumped to, and jump again if this one won't make it.
		const FPathBlock& Aim = Blocks[AutoTarget];
		const float Margin = FMath::Min(80.f, (Aim.GetRight() - Aim.GetLeft()) * 0.4f);
		const float Wanted = FMath::Clamp(Here.Y, Aim.GetLeft() + Margin, Aim.GetRight() - Margin);
		const float Above = Feet - Aim.GetTop();
		const float Pull = Gravity * FallGravityScale;
		const float Time = Above > 0.f ? (Velocity.Y + FMath::Sqrt(FMath::Max(0.f, Velocity.Y * Velocity.Y + 2.f * Pull * Above))) / Pull : 0.f;
		if (Velocity.Y < 60.f && JumpsUsed < 2 && (Above <= 0.f || Wanted - Here.Y > RunSpeed * Time))
		{
			Press(0.4f);
		}
		return FVector2D(FMath::Clamp((Wanted - Here.Y) / 25.f, -1.f, 1.f), 0.f);
	}
	else if (Velocity.Y < -80.f && JumpsUsed < 2)
	{
		// Hopping spikes: again if it would come down on them.
		for (const FPathSpikes& Row : Course->GetSpikes())
		{
			if (Here.Y > Row.Left - 60.f && Here.Y < Row.Right + 10.f && FMath::Abs(Feet - Row.Base - 90.f) < 60.f)
			{
				Press(0.3f);
			}
		}
	}
	return FVector2D(Move, 0.f);
}
#endif
