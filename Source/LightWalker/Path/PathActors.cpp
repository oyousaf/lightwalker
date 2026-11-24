#include "Path/PathActors.h"

#include "Components/StaticMeshComponent.h"
#include "Core/LWFeel.h"
#include "Core/LWNeon.h"
#include "Core/LWSound.h"
#include "Effects/LWParticles.h"
#include "Engine/StaticMesh.h"
#include "Path/PathPawn.h"
#include "UObject/ConstructorHelpers.h"

APathOrb::APathOrb()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	auto MakeSphere = [this](const TCHAR* Name, float Scale)
	{
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetStaticMesh(Sphere.Object);
		Mesh->SetRelativeScale3D(FVector(Scale));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->bAffectDynamicIndirectLighting = false;
		Mesh->bAffectDistanceFieldLighting = false;
		return Mesh;
	};
	Shell = MakeSphere(TEXT("Shell"), 0.3f);
	Core = MakeSphere(TEXT("Core"), 0.13f);
}

void APathOrb::BeginPlay()
{
	Super::BeginPlay();
	LWNeon::ApplyGlow(Shell, FLinearColor(1.f, 0.6f, 0.12f), 1.5f);
	LWNeon::ApplyGlow(Core, FLinearColor(1.f, 0.9f, 0.6f), 8.f);
	Age = GetActorLocation().Y * 0.004f;  // neighbours bob in a wave
}

void APathOrb::Restore()
{
	bTaken = false;
	SetActorHiddenInGame(false);
}

void APathOrb::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bTaken)
	{
		return;
	}

	Age += DeltaSeconds;
	const FVector Bob(0.f, 0.f, FMath::Sin(Age * 3.2f) * 9.f);
	Shell->SetRelativeLocation(Bob);
	Core->SetRelativeLocation(Bob - FVector(16.f, 0.f, 0.f));
	Shell->SetRelativeScale3D(FVector(0.3f + 0.03f * FMath::Sin(Age * 5.f)));

	APathPawn* Lightwalker = Player.Get();
	if (Lightwalker && !Lightwalker->IsDowned() && Lightwalker->IsWithin(GetActorLocation(), CollectRadius))
	{
		bTaken = true;
		SetActorHiddenInGame(true);

		FLWEmit Rise;
		Rise.Count = 14;
		Rise.Origin = GetActorLocation();
		Rise.Speed = FVector2D(120.f, 330.f);
		Rise.Life = FVector2D(0.25f, 0.5f);
		Rise.Size = FVector2D(0.03f, 0.065f);
		Rise.Drag = 4.f;
		Rise.Gravity = -220.f;
		LWFX::Burst(GetWorld(), LWFX::EShape::Sphere, FLinearColor(1.f, 0.85f, 0.45f), 11.f, { Rise });
		ULWSoundSubsystem::Play(this, ELWSound::Orb);
		ULWFeelSubsystem::Play(this, ELWFeel::Pickup);

		const bool bFirst = !bScored;
		bScored = true;
		if (OnTaken)
		{
			OnTaken(this, bFirst);
		}
	}
}

void APathShaytaan::SetPatrol(float Left, float Right)
{
	PatrolLeft = Left;
	PatrolRight = Right;
}

void APathShaytaan::UpdateMovement(float DeltaSeconds)
{
	FVector Location = GetActorLocation();
	const ALightWalkerPawn* LightWalker = GetPlayer();
	const FVector To = LightWalker ? LightWalker->GetActorLocation() - Location : FVector::ZeroVector;

	// Charges along its own ground, and paces when there's no one to charge at.
	const bool bNoticed = LightWalker && !LightWalker->IsDowned() && FMath::Abs(To.Y) < NoticeDistance && FMath::Abs(To.Z) < 260.f;
	if (bNoticed && FMath::Abs(To.Y) > 10.f)
	{
		Direction = FMath::Sign(To.Y);
	}
	MoveSpeed = bNoticed ? ChargeSpeed : PaceSpeed;
	Location.Y += Direction * MoveSpeed * DeltaSeconds;
	if (Location.Y <= PatrolLeft || Location.Y >= PatrolRight)
	{
		Location.Y = FMath::Clamp(Location.Y, PatrolLeft, PatrolRight);
		Direction = Location.Y <= PatrolLeft ? 1.f : -1.f;
	}
	SetActorLocation(Location);
}

void APathDajjal::UpdateMovement(float DeltaSeconds)
{
	if (!bHomeSet)
	{
		Home = GetActorLocation();
		bHomeSet = true;
	}
	SetActorLocation(Home + FVector(0.f, FMath::Sin(LookTime * 0.9f) * HoverRange.X, FMath::Sin(LookTime * 1.7f) * HoverRange.Y));
}
