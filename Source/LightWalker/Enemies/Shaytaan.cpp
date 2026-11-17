#include "Enemies/Shaytaan.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Player/LightWalkerPawn.h"
#include "UObject/ConstructorHelpers.h"

AShaytaan::AShaytaan()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

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

	// Pitch 70 points a +Z shape backwards and a little up.
	UStaticMeshComponent* Tail = AddShape(TEXT("Tail"), CylinderMesh.Object, nullptr,
		FVector(-48.f, 0.f, 0.f), FRotator(70.f, 0.f, 0.f), FVector(0.05f, 0.05f, 0.32f));
	UStaticMeshComponent* TailTip = AddShape(TEXT("TailTip"), ConeMesh.Object, nullptr,
		FVector(-70.f, 0.f, 8.f), FRotator(70.f, 0.f, 0.f), FVector(0.15f, 0.15f, 0.15f));

	for (UStaticMeshComponent* Part : { Body, HornLeft, HornRight, Tail, TailTip })
	{
		AddGlowPart(Part, BodyColor, 1.3f);
	}
	AddGlowPart(EyeLeft, EyeColor, 12.f);
	AddGlowPart(EyeRight, EyeColor, 12.f);
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
	Visual->SetRelativeLocation(FVector(0.f, 0.f, FMath::Abs(FMath::Sin(LookTime * Rate)) * BobHeight));
}
