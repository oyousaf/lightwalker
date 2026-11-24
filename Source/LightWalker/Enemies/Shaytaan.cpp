#include "Enemies/Shaytaan.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Core/LWNeon.h"
#include "Player/LightWalkerPawn.h"

namespace
{
	constexpr int32 TrailCount = 14;

	/** 1 at the devil, towards 0 at the tip. */
	float TrailFade(int32 Index)
	{
		return 1.f - static_cast<float>(Index) / TrailCount;
	}
}

AShaytaan::AShaytaan()
{
	ModelPath = FSoftObjectPath(TEXT("/Game/Enemies/SM_Shaytaan.SM_Shaytaan"));
	ModelSize = 130.f;

	Trail = LWNeon::MakeInstances(this, TEXT("Trail"), ELWShape::Sphere, Collision);
	LWNeon::SetWorldPlaced(Trail);
	Trail->NumCustomDataFloats = 1;
}

void AShaytaan::BeginPlay()
{
	Super::BeginPlay();

	TrailPoints.Init(GetActorLocation(), TrailCount);
	TrailWisps.Init(FTransform(FQuat::Identity, GetActorLocation(), FVector(0.001f)), TrailCount);
	Trail->AddInstances(TrailWisps, false, true);
	for (int32 Index = 0; Index < TrailCount; ++Index)
	{
		Trail->SetCustomDataValue(Index, 0, FMath::Lerp(0.1f, 0.9f, FMath::Square(TrailFade(Index))));
	}
	LWNeon::ApplyGlow(Trail, TrailColor, 1.f);
}

void AShaytaan::UpdateMovement(float DeltaSeconds)
{
	const ALightWalkerPawn* LightWalker = GetPlayer();
	if (!LightWalker)
	{
		return;
	}

	// While Lightwalker is down, the pack keeps closing on the spot where they fell.
	const FVector Location = GetActorLocation();
	FVector ToTarget = LightWalker->GetActorLocation() - Location;
	ToTarget.Z = 0.f;

	const float Distance = ToTarget.Size();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Step = MoveSpeed * DeltaSeconds;
	SetActorLocation(Location + ToTarget * (FMath::Min(Step, Distance) / Distance));
}

void AShaytaan::UpdateLook(float DeltaSeconds)
{
	if (const ALightWalkerPawn* LightWalker = GetPlayer())
	{
		TurnVisualTowards(LightWalker->GetActorLocation() - GetActorLocation(), DeltaSeconds, 8.f);
	}

	// Hops faster as the pack speeds up.
	const float Rate = BobRate * FMath::Max(1.f, MoveSpeed / 136.f);
	const float Stride = FMath::Sin(LookTime * Rate);
	const float Hop = FMath::Abs(Stride);
	Visual->SetRelativeLocation(FVector(0.f, 0.f, Hop * BobHeight));

	// Waddle: lean onto one foot then the other with each hop, hunched forward, squashing a little on landing.
	PoseModel(FRotator(-WaddleLean, 0.f, Stride * WaddleRock), FMath::Pow(1.f - Hop, 6.f) * LandingSquash);

	UpdateTrail(DeltaSeconds);
}

void AShaytaan::UpdateTrail(float DeltaSeconds)
{
	// Follow-the-leader chain: each point stays at most TrailSpacing behind the one ahead, and drifts
	// towards it over time so the trail shrinks back into the body when the devil stops.
	FVector Leader = GetActorLocation();
	for (FVector& Point : TrailPoints)
	{
		FVector Location = FMath::VInterpTo(Point, Leader, DeltaSeconds, TrailRetractRate);
		const FVector Offset = Location - Leader;
		if (Offset.SizeSquared() > FMath::Square(TrailSpacing))
		{
			Location = Leader + Offset.GetSafeNormal() * TrailSpacing;
		}
		Point = Location;
		Leader = Location;
	}

	// Drawn with a wavering, flickering wobble that grows towards the tip, so it reads as smoke rather than
	// a neat row of beads.
	const FVector Back = (GetActorLocation() - TrailPoints.Last()).GetSafeNormal2D();
	const FVector Side(-Back.Y, Back.X, 0.f);
	for (int32 Index = 0; Index < TrailCount; ++Index)
	{
		const float Along = static_cast<float>(Index) / (TrailCount - 1);
		const float Sway = FMath::Sin(LookTime * 9.f + Index * 0.8f) * Along * TrailWobble;
		const float Rise = FMath::Sin(LookTime * 6.f + Index * 1.3f) * Along * TrailWobble * 0.5f;
		const float Flicker = 0.75f + 0.25f * FMath::Sin(LookTime * 23.f + Index * 1.9f);
		TrailWisps[Index].SetLocation(TrailPoints[Index] + Side * Sway + FVector(0.f, 0.f, Rise));
		TrailWisps[Index].SetScale3D(FVector(FMath::Lerp(0.03f, 0.17f, TrailFade(Index)) * Flicker));
	}
	Trail->BatchUpdateInstancesTransforms(0, TrailWisps, true, true, true);
}
