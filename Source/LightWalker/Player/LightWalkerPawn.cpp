#include "Player/LightWalkerPawn.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Player/Laser.h"
#include "UObject/ConstructorHelpers.h"

ALightWalkerPawn::ALightWalkerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(ContactRadius);
	// Contact with enemies is a distance check (see AEnemyBase), so the pawn needs no physics collision.
	Collision->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	RootComponent = Collision;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
	Body->SetStaticMesh(SphereMesh.Object);
	Body->SetRelativeScale3D(FVector(0.8f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);

	HeadingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HeadingPivot"));
	HeadingPivot->SetupAttachment(Collision);

	// The cone mesh points up +Z; pitch it down so it points along the pivot's +X.
	Pointer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pointer"));
	Pointer->SetupAttachment(HeadingPivot);
	Pointer->SetStaticMesh(ConeMesh.Object);
	Pointer->SetRelativeLocation(FVector(58.f, 0.f, 0.f));
	Pointer->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Pointer->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.3f));
	Pointer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pointer->SetCastShadow(false);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Collision);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(30.f);
	Glow->SetAttenuationRadius(700.f);
	Glow->SetLightColor(Color);
	Glow->SetCastShadows(false);

	LaserClass = ALaser::StaticClass();
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ALightWalkerPawn::BeginPlay()
{
	Super::BeginPlay();

	LWNeon::ApplyGlow(Body, Color, GlowIntensity);
	LWNeon::ApplyGlow(Pointer, Color, GlowIntensity);
	Glow->SetLightColor(Color);
}

void ALightWalkerPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDowned)
	{
		return;
	}

	if (GraceRemaining > 0.f)
	{
		GraceRemaining -= DeltaSeconds;
		const bool bBlinkOn = FMath::Frac(GraceRemaining * GraceBlinkRate * 0.5f) > 0.5f;
		SetBodyVisible(GraceRemaining <= 0.f || bBlinkOn);
	}

	const FVector Move = ScreenToWorld(MoveInput.GetClampedToMaxSize(1.f));
	if (!Move.IsNearlyZero())
	{
		FVector Location = GetActorLocation() + Move * MoveSpeed * DeltaSeconds;
		if (PlayBounds.bIsValid)
		{
			Location.X = FMath::Clamp(Location.X, PlayBounds.Min.X, PlayBounds.Max.X);
			Location.Y = FMath::Clamp(Location.Y, PlayBounds.Min.Y, PlayBounds.Max.Y);
		}
		SetActorLocation(Location);
	}

	FireCooldown = FMath::Max(0.f, FireCooldown - DeltaSeconds);

	if (!Move.IsNearlyZero())
	{
		Heading = Move.GetSafeNormal();
	}

	FVector FireDirection = FVector::ZeroVector;
	if (AimInput.Size() >= AimThreshold)
	{
		FireDirection = ScreenToWorld(AimInput).GetSafeNormal();
	}
	else if (bCursorAim)
	{
		FireDirection = (CursorTarget - GetActorLocation()).GetSafeNormal2D();
	}
	else if (bFireHeld)
	{
		FireDirection = Heading;
	}

	if (!FireDirection.IsNearlyZero())
	{
		FaceDirection(FireDirection);
		if (FireCooldown <= 0.f)
		{
			Fire(FireDirection);
			FireCooldown = FireInterval;
		}
	}
	else
	{
		FaceDirection(Heading);
	}
}

void ALightWalkerPawn::Fire(const FVector& Direction)
{
	UWorld* World = GetWorld();
	if (!World || !LaserClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector Start = GetActorLocation() + Direction * (ContactRadius + 10.f);
	if (ALaser* Laser = World->SpawnActor<ALaser>(LaserClass, Start, Direction.Rotation(), Params))
	{
		Laser->Launch(Direction, PlayBounds);
	}
}

void ALightWalkerPawn::FaceDirection(const FVector& Direction)
{
	HeadingPivot->SetWorldRotation(Direction.Rotation());
}

void ALightWalkerPawn::SetBodyVisible(bool bVisible)
{
	Body->SetVisibility(bVisible);
	Pointer->SetVisibility(bVisible);
}

void ALightWalkerPawn::SetDowned(bool bInDowned)
{
	bDowned = bInDowned;
	SetActorHiddenInGame(bDowned);
	Glow->SetVisibility(!bDowned);
	if (bDowned)
	{
		GraceRemaining = 0.f;
		FireCooldown = 0.f;
	}
	SetBodyVisible(true);
}

void ALightWalkerPawn::Revive(const FVector& Location, float GraceSeconds)
{
	SetActorLocation(FVector(Location.X, Location.Y, GetActorLocation().Z));
	SetDowned(false);
	GraceRemaining = FMath::Max(0.f, GraceSeconds);
}
