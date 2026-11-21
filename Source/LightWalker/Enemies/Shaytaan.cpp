#include "Enemies/Shaytaan.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Player/LightWalkerPawn.h"
#include "UObject/ConstructorHelpers.h"

AShaytaan::AShaytaan()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

	Radius = 50.f;

	// Facing +X. Basic shapes are 100 units across; cones point up their +Z.
	UStaticMeshComponent* Body = AddShape(TEXT("Body"), SphereMesh.Object, nullptr,
		FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.7f, 0.7f, 0.62f));

	// Horns sweep forwards and outwards rather than up, so from the top-down camera they break the round outline.
	auto MakeHorn = [&](FName Name, float Side)
	{
		const FVector Base(10.f, 18.f * Side, 18.f);
		const FVector Direction = FVector(0.55f, 0.7f * Side, 0.45f).GetSafeNormal();
		constexpr float Length = 48.f;
		return AddShape(Name, ConeMesh.Object, nullptr, Base + Direction * (Length * 0.5f),
			FRotationMatrix::MakeFromZ(Direction).Rotator(), FVector(0.2f, 0.2f, Length / 100.f));
	};
	UStaticMeshComponent* HornLeft = MakeHorn(TEXT("HornLeft"), -1.f);
	UStaticMeshComponent* HornRight = MakeHorn(TEXT("HornRight"), 1.f);

	// Angry slanted slits on the upper front of the head, visible from the tilted camera.
	UStaticMeshComponent* EyeLeft = AddShape(TEXT("EyeLeft"), SphereMesh.Object, nullptr,
		FVector(25.f, -11.f, 17.f), FRotator(0.f, 0.f, -25.f), FVector(0.07f, 0.15f, 0.06f));
	UStaticMeshComponent* EyeRight = AddShape(TEXT("EyeRight"), SphereMesh.Object, nullptr,
		FVector(25.f, 11.f, 17.f), FRotator(0.f, 0.f, 25.f), FVector(0.07f, 0.15f, 0.06f));

	// Fading trail: small wisps placed in world space each frame, each following the one ahead of it.
	constexpr int32 TrailCount = 14;
	for (int32 Index = 0; Index < TrailCount; ++Index)
	{
		const float Fade = 1.f - static_cast<float>(Index) / TrailCount;
		UStaticMeshComponent* Segment = AddShape(*FString::Printf(TEXT("Trail%d"), Index), SphereMesh.Object, Collision,
			FVector::ZeroVector, FRotator::ZeroRotator, FVector(FMath::Lerp(0.03f, 0.17f, Fade)));
		Segment->SetUsingAbsoluteLocation(true);
		Segment->SetUsingAbsoluteRotation(true);
		TrailSegments.Add(Segment);
		TrailPoints.Add(FVector::ZeroVector);
		TrailScales.Add(FMath::Lerp(0.03f, 0.17f, Fade));
		AddGlowPart(Segment, BodyColor, FMath::Lerp(0.1f, 0.9f, Fade * Fade));
	}

	ModelPath = FSoftObjectPath(TEXT("/Game/Enemies/SM_Shaytaan.SM_Shaytaan"));
	ModelSize = 130.f;
	for (USceneComponent* Placeholder : { Body, HornLeft, HornRight, EyeLeft, EyeRight })
	{
		AddPlaceholder(Placeholder);
	}

	for (UStaticMeshComponent* Part : { Body, HornLeft, HornRight })
	{
		AddGlowPart(Part, BodyColor, 1.3f);
	}
	AddGlowPart(EyeLeft, EyeColor, 12.f);
	AddGlowPart(EyeRight, EyeColor, 12.f);
}

void AShaytaan::BeginPlay()
{
	Super::BeginPlay();

	for (UStaticMeshComponent* Segment : TrailSegments)
	{
		Segment->SetWorldLocation(GetActorLocation());
	}
	for (FVector& Point : TrailPoints)
	{
		Point = GetActorLocation();
	}
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
	const FVector Back = (TrailPoints.Num() > 0 ? GetActorLocation() - TrailPoints.Last() : FVector::ZeroVector).GetSafeNormal2D();
	const FVector Side(-Back.Y, Back.X, 0.f);
	for (int32 Index = 0; Index < TrailSegments.Num(); ++Index)
	{
		const float Along = static_cast<float>(Index) / FMath::Max(1, TrailSegments.Num() - 1);
		const float Sway = FMath::Sin(LookTime * 9.f + Index * 0.8f) * Along * TrailWobble;
		const float Rise = FMath::Sin(LookTime * 6.f + Index * 1.3f) * Along * TrailWobble * 0.5f;
		TrailSegments[Index]->SetWorldLocation(TrailPoints[Index] + Side * Sway + FVector(0.f, 0.f, Rise));
		const float Flicker = 0.75f + 0.25f * FMath::Sin(LookTime * 23.f + Index * 1.9f);
		TrailSegments[Index]->SetRelativeScale3D(FVector(TrailScales[Index] * Flicker));
	}
}
