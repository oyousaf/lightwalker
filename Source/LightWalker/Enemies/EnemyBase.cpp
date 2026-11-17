#include "Enemies/EnemyBase.h"

#include "Arena/ArenaGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Player/LightWalkerPawn.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(Radius);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Collision->SetGenerateOverlapEvents(true);
	RootComponent = Collision;

	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);

	Model = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(Visual);
	Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Model->SetCastShadow(false);
	Model->SetVisibility(false);
}

void AEnemyBase::TryUseModel()
{
	if (ModelPath.IsNull() || !FPackageName::DoesPackageExist(ModelPath.GetLongPackageName()))
	{
		return;
	}

	UStaticMesh* Mesh = Cast<UStaticMesh>(ModelPath.TryLoad());
	if (!Mesh)
	{
		return;
	}

	// Fit the model: largest side = ModelSize, bounds centred on the pivot, front turned to +X.
	const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
	const float Largest = FMath::Max(MeshBounds.BoxExtent.GetMax() * 2.f, KINDA_SMALL_NUMBER);
	const float Scale = ModelSize / Largest;
	const FRotator Rotation(0.f, ModelYaw, 0.f);

	Model->SetStaticMesh(Mesh);
	Model->SetRelativeScale3D(FVector(Scale));
	Model->SetRelativeRotation(Rotation);
	Model->SetRelativeLocation(-Rotation.RotateVector(MeshBounds.Origin * Scale));
	Model->SetVisibility(true);

	for (USceneComponent* Placeholder : Placeholders)
	{
		if (Placeholder)
		{
			Placeholder->SetVisibility(false, true);
		}
	}
	bUsingModel = true;
}

UStaticMeshComponent* AEnemyBase::AddShape(FName Name, UStaticMesh* Mesh, USceneComponent* Parent,
	const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	UStaticMeshComponent* Shape = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Shape->SetupAttachment(Parent ? Parent : Visual.Get());
	Shape->SetStaticMesh(Mesh);
	Shape->SetRelativeLocation(Location);
	Shape->SetRelativeRotation(Rotation);
	Shape->SetRelativeScale3D(Scale);
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape->SetCastShadow(false);
	return Shape;
}

UStaticMeshComponent* AEnemyBase::AddRod(FName Name, UStaticMesh* CylinderMesh, USceneComponent* Parent, const FVector& Start, const FVector& End, float Thickness)
{
	// The cylinder is 100 units tall along +Z, centred on its pivot.
	const FVector Span = End - Start;
	return AddShape(Name, CylinderMesh, Parent, (Start + End) * 0.5f, FRotationMatrix::MakeFromZ(Span).Rotator(),
		FVector(Thickness / 100.f, Thickness / 100.f, Span.Size() / 100.f));
}

void AEnemyBase::AddGlowPart(UMeshComponent* Mesh, const FLinearColor& InColor, float Intensity, bool bDark, int32 Element)
{
	FEnemyGlowPart& Part = GlowParts.AddDefaulted_GetRef();
	Part.Mesh = Mesh;
	Part.Color = InColor;
	Part.Intensity = Intensity;
	Part.bDark = bDark;
	Part.Element = Element;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	Collision->SetSphereRadius(Radius);
	LookTime = FMath::FRandRange(0.f, 10.f);
	TryUseModel();

	for (const FEnemyGlowPart& Part : GlowParts)
	{
		if (Part.bDark)
		{
			LWNeon::ApplyGlow(Part.Mesh, FLinearColor::Black, 0.f, Part.Element);
		}
		else
		{
			LWNeon::ApplyGlow(Part.Mesh, Part.Color, Part.Intensity, Part.Element);
		}
	}
}

void AEnemyBase::InitEnemy(ALightWalkerPawn* InPlayer, const FBox2D& InBounds, float InMoveSpeed)
{
	Player = InPlayer;
	Bounds = InBounds;
	MoveSpeed = InMoveSpeed;
}

void AEnemyBase::TurnVisualTowards(const FVector& Direction, float DeltaSeconds, float TurnRate)
{
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator Current = Visual->GetComponentRotation();
	const FRotator Target(0.f, Direction.Rotation().Yaw, 0.f);
	Visual->SetWorldRotation(FMath::RInterpTo(Current, Target, DeltaSeconds, TurnRate));
}

void AEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	LookTime += DeltaSeconds;
	UpdateLook(DeltaSeconds);

	if (!bActive || bDead)
	{
		return;
	}

	UpdateMovement(DeltaSeconds);
	CheckPlayerContact();
}

void AEnemyBase::CheckPlayerContact()
{
	// A distance check every frame rather than an overlap event, so an enemy still sitting on the player
	// when their respawn grace runs out still counts as a hit.
	ALightWalkerPawn* LightWalker = Player.Get();
	if (!LightWalker || !LightWalker->IsVulnerable())
	{
		return;
	}

	const float ContactDistance = Radius + LightWalker->GetContactRadius();
	if (FVector::DistSquared2D(GetActorLocation(), LightWalker->GetActorLocation()) <= FMath::Square(ContactDistance))
	{
		if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
		{
			GameMode->HandlePlayerTouched(LightWalker);
		}
	}
}

void AEnemyBase::Kill()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
	{
		GameMode->HandleEnemyKilled(this);
	}
	Destroy();
}
