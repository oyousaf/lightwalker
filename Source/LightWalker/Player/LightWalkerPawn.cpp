#include "Player/LightWalkerPawn.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LightWalkerGameInstance.h"
#include "Core/LWNeon.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "Player/Laser.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 ShieldMoteCount = 8;
	constexpr float ShieldRadius = 70.f;

	// Where the lantern hangs, relative to the heading pivot: out in front, on the right hand side.
	const FVector LanternOffset(52.f, 26.f, 14.f);

	const FLinearColor PiercingColor(1.f, 0.85f, 0.4f);
}

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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	HeadingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HeadingPivot"));
	HeadingPivot->SetupAttachment(Collision);

	auto MakePart = [this](const TCHAR* Name, UStaticMesh* Mesh, USceneComponent* Parent, const FVector& Location,
		const FRotator& Rotation, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeRotation(Rotation);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		return Part;
	};

	// Basic shapes are 100 units across, centred on their pivot; the cone and cylinder run along +Z.
	Body = MakePart(TEXT("Body"), ConeMesh.Object, HeadingPivot, FVector(-4.f, 0.f, -8.f), FRotator::ZeroRotator, FVector(0.72f, 0.62f, 0.85f));
	Head = MakePart(TEXT("Head"), SphereMesh.Object, HeadingPivot, FVector(0.f, 0.f, 42.f), FRotator::ZeroRotator, FVector(0.34f));
	Staff = MakePart(TEXT("Staff"), CylinderMesh.Object, HeadingPivot, FVector(14.f, 26.f, 10.f), FRotator(-80.f, 0.f, 0.f), FVector(0.05f, 0.05f, 0.85f));
	Lantern = MakePart(TEXT("Lantern"), SphereMesh.Object, HeadingPivot, LanternOffset, FRotator::ZeroRotator, FVector(0.26f));

	ShieldPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ShieldPivot"));
	ShieldPivot->SetupAttachment(Collision);
	for (int32 Index = 0; Index < ShieldMoteCount; ++Index)
	{
		const FVector Location = FRotator(0.f, 360.f * Index / ShieldMoteCount, 0.f).RotateVector(FVector(ShieldRadius, 0.f, 10.f));
		UStaticMeshComponent* Mote = MakePart(*FString::Printf(TEXT("ShieldMote%d"), Index), SphereMesh.Object, ShieldPivot,
			Location, FRotator::ZeroRotator, FVector(0.11f));
		Mote->SetVisibility(false);
		ShieldMotes.Add(Mote);
	}

	Model = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(HeadingPivot);
	Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Model->SetCastShadow(false);
	Model->SetVisibility(false);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(HeadingPivot);
	Glow->SetRelativeLocation(LanternOffset + FVector(0.f, 0.f, 26.f));
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(30.f);
	Glow->SetAttenuationRadius(LightRadius);
	Glow->SetLightColor(Color);
	Glow->SetCastShadows(false);

	LaserClass = ALaser::StaticClass();
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ALightWalkerPawn::BeginPlay()
{
	Super::BeginPlay();

	LWNeon::ApplyGlow(Body, Color, GlowIntensity * 0.35f);
	LWNeon::ApplyGlow(Head, Color, GlowIntensity * 0.6f);
	LWNeon::ApplyGlow(Staff, FLinearColor(0.75f, 0.9f, 1.f), GlowIntensity * 0.5f);
	LWNeon::ApplyGlow(Lantern, FLinearColor(0.85f, 1.f, 1.f), GlowIntensity * 3.f);
	for (UStaticMeshComponent* Mote : ShieldMotes)
	{
		LWNeon::ApplyGlow(Mote, LWPickups::GetColor(EPickupType::Shield), 10.f);
	}
	Glow->SetLightColor(Color);

	TryUseModel();
}

void ALightWalkerPawn::TryUseModel()
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

	// Scale by height (the staff can make the model wider than it is tall) and centre it on the pivot.
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const float Scale = ModelHeight / FMath::Max(Bounds.BoxExtent.Z * 2.f, KINDA_SMALL_NUMBER);
	const FRotator Rotation(0.f, ModelYaw, 0.f);
	ModelOffset = -Rotation.RotateVector(Bounds.Origin * Scale) + FVector(0.f, 0.f, ModelHeight * 0.5f - ContactRadius);

	Model->SetStaticMesh(Mesh);
	Model->SetRelativeScale3D(FVector(Scale));
	Model->SetRelativeRotation(Rotation);
	Model->SetRelativeLocation(ModelOffset);
	Model->SetVisibility(true);
	for (int32 Slot = 0; Slot < Model->GetNumMaterials(); ++Slot)
	{
		if (UMaterialInstanceDynamic* Material = Model->CreateDynamicMaterialInstance(Slot))
		{
			Material->SetScalarParameterValue(FName(TEXT("Glow")), ModelGlow);
		}
	}

	// The model brings its own robe, staff and lantern; the light stays where the placeholder lantern was.
	// Hidden in game rather than invisible, so the respawn blink (which toggles visibility) leaves them hidden.
	for (UStaticMeshComponent* Placeholder : { Body.Get(), Head.Get(), Staff.Get(), Lantern.Get() })
	{
		Placeholder->SetHiddenInGame(true);
	}
	bUsingModel = true;

	// The lantern burns warm white, as in the concept, so the robe keeps its teal and gold instead of turning cyan.
	Glow->SetLightColor(LanternLightColor);
}

void ALightWalkerPawn::UpdateModelStride(bool bMoving, float DeltaSeconds)
{
	if (!bUsingModel)
	{
		return;
	}

	// A static mesh can't walk, so it bobs with each stride and leans into the run, easing in and out.
	StrideBlend = FMath::FInterpTo(StrideBlend, bMoving ? 1.f : 0.f, DeltaSeconds, 8.f);
	StrideTime += DeltaSeconds * 11.f * StrideBlend;
	const float Stride = FMath::Sin(StrideTime);
	const FQuat Lean = FRotator(-7.f * StrideBlend, 0.f, Stride * 3.f * StrideBlend).Quaternion();
	const FVector Feet(0.f, 0.f, -ContactRadius);

	Model->SetRelativeRotation(Lean * FRotator(0.f, ModelYaw, 0.f).Quaternion());
	Model->SetRelativeLocation(Feet + Lean.RotateVector(ModelOffset - Feet) + FVector(0.f, 0.f, FMath::Abs(Stride) * 5.f * StrideBlend));
}

ULightWalkerGameInstance* ALightWalkerPawn::GetRun() const
{
	return GetGameInstance<ULightWalkerGameInstance>();
}

void ALightWalkerPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PickupMessageAge += DeltaSeconds;

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

	UpdatePowerUps(DeltaSeconds);

	const FRunUpgrades Upgrades = GetRun() ? GetRun()->GetUpgrades() : FRunUpgrades();

	const FVector Move = ScreenToWorld(MoveInput.GetClampedToMaxSize(1.f));
	if (!Move.IsNearlyZero())
	{
		FVector Location = GetActorLocation() + Move * MoveSpeed * Upgrades.MoveSpeed * DeltaSeconds;
		if (PlayBounds.bIsValid)
		{
			Location.X = FMath::Clamp(Location.X, PlayBounds.Min.X, PlayBounds.Max.X);
			Location.Y = FMath::Clamp(Location.Y, PlayBounds.Min.Y, PlayBounds.Max.Y);
		}
		SetActorLocation(Location);
	}

	FireCooldown = FMath::Max(0.f, FireCooldown - DeltaSeconds);
	UpdateModelStride(!Move.IsNearlyZero(), DeltaSeconds);

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
			const float Rate = Upgrades.FireRate * (GetPowerUpTimeLeft(EPickupType::LightOrb) > 0.f ? LightOrbFireRate : 1.f);
			FireCooldown = FireInterval / Rate;
		}
	}
	else
	{
		FaceDirection(Heading);
	}
}

void ALightWalkerPawn::UpdatePowerUps(float DeltaSeconds)
{
	for (float& Time : PowerTime)
	{
		Time = FMath::Max(0.f, Time - DeltaSeconds);
	}

	// The light grows with the light orb and with book upgrades, and eases back when the orb runs out.
	const float RunScale = GetRun() ? GetRun()->GetUpgrades().LightRadius : 1.f;
	const float OrbScale = GetPowerUpTimeLeft(EPickupType::LightOrb) > 0.f ? LightOrbScale : 1.f;
	const float TargetRadius = LightRadius * RunScale * OrbScale;
	Glow->SetAttenuationRadius(FMath::FInterpTo(Glow->AttenuationRadius, TargetRadius, DeltaSeconds, 4.f));
	Glow->SetIntensity(30.f * OrbScale);
	Lantern->SetRelativeScale3D(FVector(0.26f * FMath::Sqrt(OrbScale)));

	ShieldPivot->AddLocalRotation(FRotator(0.f, 160.f * DeltaSeconds, 0.f));
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

	const bool bOrb = GetPowerUpTimeLeft(EPickupType::LightOrb) > 0.f;
	const bool bPiercing = GetPowerUpTimeLeft(EPickupType::PiercingBeam) > 0.f;
	const bool bSpread = GetPowerUpTimeLeft(EPickupType::SpreadShot) > 0.f;
	const float BoltSpeed = GetRun() ? GetRun()->GetUpgrades().BoltSpeed : 1.f;
	const bool bPowered = bOrb || bPiercing || BoltSpeed > 1.f;

	// Bolts leave from the lantern, pulled in line with the centre so close-range shots still land.
	const FVector Lantern2D = HeadingPivot->GetComponentTransform().TransformPosition(FVector(LanternOffset.X, LanternOffset.Y * 0.4f, 0.f));
	const FVector Start(Lantern2D.X, Lantern2D.Y, GetActorLocation().Z);

	for (const float Yaw : bSpread ? TArray<float>{ -SpreadAngle, 0.f, SpreadAngle } : TArray<float>{ 0.f })
	{
		const FVector BoltDirection = FRotator(0.f, Yaw, 0.f).RotateVector(Direction);
		if (ALaser* Laser = World->SpawnActor<ALaser>(LaserClass, Start, BoltDirection.Rotation(), Params))
		{
			Laser->Launch(BoltDirection, PlayBounds);
			if (bPowered)
			{
				Laser->SetPower(bOrb ? LightOrbScale : 1.f, BoltSpeed, bPiercing,
					bPiercing ? PiercingColor : FLinearColor(0.75f, 0.95f, 1.f));
			}
		}
	}
}

void ALightWalkerPawn::CollectPickup(EPickupType Type)
{
	const FString Title = LWPickups::GetName(Type);
	switch (Type)
	{
	case EPickupType::LightOrb:
		PowerTime[static_cast<int32>(Type)] = LightOrbDuration;
		ShowPickupMessage(Title, TEXT("Bigger light, bigger bolts, faster firing"));
		break;
	case EPickupType::SpreadShot:
		PowerTime[static_cast<int32>(Type)] = SpreadShotDuration;
		ShowPickupMessage(Title, TEXT("Three bolts at once"));
		break;
	case EPickupType::PiercingBeam:
		PowerTime[static_cast<int32>(Type)] = PiercingBeamDuration;
		ShowPickupMessage(Title, TEXT("Bolts pass through everything"));
		break;
	case EPickupType::Shield:
		bShield = true;
		for (UStaticMeshComponent* Mote : ShieldMotes)
		{
			Mote->SetVisibility(true);
		}
		ShowPickupMessage(Title, TEXT("Blocks the next hit"));
		break;
	case EPickupType::Book:
		if (ULightWalkerGameInstance* Run = GetRun())
		{
			const FBookReading Reading = Run->ReadBook();
			ShowPickupMessage(FString::Printf(TEXT("%s:  %s"), *Title, *Reading.Upgrade), FString::Printf(TEXT("\"%s\""), *Reading.Wisdom));
		}
		break;
	default:
		break;
	}
}

void ALightWalkerPawn::ShowPickupMessage(const FString& Title, const FString& Detail)
{
	PickupTitle = Title;
	PickupDetail = Detail;
	PickupMessageAge = 0.f;
}

bool ALightWalkerPawn::TryBlockHit()
{
	if (!bShield)
	{
		return false;
	}

	bShield = false;
	for (UStaticMeshComponent* Mote : ShieldMotes)
	{
		Mote->SetVisibility(false);
	}
	GraceRemaining = FMath::Max(GraceRemaining, ShieldGrace);
	return true;
}

void ALightWalkerPawn::ClearPowerUps()
{
	for (float& Time : PowerTime)
	{
		Time = 0.f;
	}
	bShield = false;
	for (UStaticMeshComponent* Mote : ShieldMotes)
	{
		Mote->SetVisibility(false);
	}
}

float ALightWalkerPawn::GetPowerUpTimeLeft(EPickupType Type) const
{
	const int32 Index = static_cast<int32>(Type);
	return Index >= 0 && Index < UE_ARRAY_COUNT(PowerTime) ? PowerTime[Index] : 0.f;
}

void ALightWalkerPawn::FaceDirection(const FVector& Direction)
{
	HeadingPivot->SetWorldRotation(Direction.Rotation());
}

void ALightWalkerPawn::SetBodyVisible(bool bVisible)
{
	HeadingPivot->SetVisibility(bVisible, true);
	Glow->SetVisibility(!bDowned);
}

void ALightWalkerPawn::SetDowned(bool bInDowned)
{
	bDowned = bInDowned;
	SetActorHiddenInGame(bDowned);
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
