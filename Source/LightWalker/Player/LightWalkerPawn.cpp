#include "Player/LightWalkerPawn.h"

#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LightWalkerGameInstance.h"
#include "Core/LightWalkerUserSettings.h"
#include "Core/LWNeon.h"
#include "Core/LWFeel.h"
#include "Core/LWSound.h"
#include "Effects/LWParticles.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "Player/Laser.h"

namespace
{
	constexpr int32 ShieldMoteCount = 8;
	constexpr float ShieldRadius = 70.f;

	// Where the lantern hangs, relative to the heading pivot: out in front, on the right hand side.
	const FVector LanternOffset(52.f, 26.f, 14.f);

	const FLinearColor PiercingColor(1.f, 0.85f, 0.4f);

	/** -LWAutoRun (non-shipping): run on the spot, turning slowly and untouchable, to check the run cycle in -LWShots screenshots (with -LWZoom). */
	bool IsDevAutoRun()
	{
#if UE_BUILD_SHIPPING
		return false;
#else
		static const bool bAutoRun = FParse::Param(FCommandLine::Get(), TEXT("LWAutoRun"));
		return bAutoRun;
#endif
	}
}

ALightWalkerPawn::ALightWalkerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(ContactRadius);
	// Contact with enemies is a distance check (see AEnemyBase), so the pawn needs no physics collision.
	Collision->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	RootComponent = Collision;

	HeadingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HeadingPivot"));
	HeadingPivot->SetupAttachment(Collision);

	GlideTrail = CreateDefaultSubobject<ULWParticleComponent>(TEXT("GlideTrail"));
	GlideTrail->SetupAttachment(Collision);

	ShieldPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ShieldPivot"));
	ShieldPivot->SetupAttachment(Collision);
	for (int32 Index = 0; Index < ShieldMoteCount; ++Index)
	{
		const FVector Location = FRotator(0.f, 360.f * Index / ShieldMoteCount, 0.f).RotateVector(FVector(ShieldRadius, 0.f, 10.f));
		UStaticMeshComponent* Mote = LWNeon::MakeMesh(this, *FString::Printf(TEXT("ShieldMote%d"), Index), ELWShape::Sphere, ShieldPivot);
		Mote->SetRelativeLocation(Location);
		Mote->SetRelativeScale3D(FVector(0.11f));
		Mote->SetVisibility(false);
		ShieldMotes.Add(Mote);
	}

	Rig = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Rig"));
	Rig->SetupAttachment(HeadingPivot);
	Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rig->SetCastShadow(false);
	Rig->SetVisibility(false);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(HeadingPivot);
	Glow->SetRelativeLocation(LanternOffset + FVector(0.f, 0.f, 26.f));
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(30.f);
	Glow->SetAttenuationRadius(LightRadius);
	Glow->SetLightColor(LanternLightColor);
	Glow->SetCastShadows(false);

	LaserClass = ALaser::StaticClass();
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ALightWalkerPawn::BeginPlay()
{
	Super::BeginPlay();

	GlideTrail->Setup(ELWShape::Sphere, LWPickups::GetColor(EPickupType::LightOrb), 10.f);
	for (UStaticMeshComponent* Mote : ShieldMotes)
	{
		LWNeon::ApplyGlow(Mote, LWPickups::GetColor(EPickupType::Shield), 10.f);
	}

	const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
	SetHero(Settings ? Settings->HeroIndex : 0);
}

bool ALightWalkerPawn::HasHeroModel(int32 Index) const
{
	const FSoftObjectPath& Path = Index == 1 ? FemaleRigPath : RigPath;
	return !Path.IsNull() && FPackageName::DoesPackageExist(Path.GetLongPackageName());
}

void ALightWalkerPawn::SetHero(int32 Index)
{
	UseModel(HasHeroModel(Index) ? Index : 1 - Index);
}

void ALightWalkerPawn::SetFrozen(bool bInFrozen)
{
	bFrozen = bInFrozen;
	MoveInput = AimInput = FVector2D::ZeroVector;
	bFireHeld = bCursorAim = false;
	if (bFrozen)
	{
		// Face the menu camera, which looks along +X.
		Heading = FVector::BackwardVector;
		FaceDirection(Heading);
	}

	// The menu shows only the sunset art, so the model stays hidden until play.
	Rig->SetHiddenInGame(bFrozen);
}

void ALightWalkerPawn::UseModel(int32 Index)
{
	USkeletalMesh* Mesh = HasHeroModel(Index) ? Cast<USkeletalMesh>((Index == 1 ? FemaleRigPath : RigPath).TryLoad()) : nullptr;
	if (!Mesh)
	{
		return;
	}

	// Scale by height (the staff can make the model wider than it is tall) and centre it on the pivot.
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const float Scale = ModelHeight / FMath::Max(Bounds.BoxExtent.Z * 2.f, KINDA_SMALL_NUMBER);
	const FRotator Rotation(0.f, ModelYaw, 0.f);
	ModelOffset = -Rotation.RotateVector(Bounds.Origin * Scale) + FVector(0.f, 0.f, ModelHeight * 0.5f - ContactRadius);

	// Drop the previous hero's material instances, or they'd carry its textures onto the new mesh.
	Rig->EmptyOverrideMaterials();
	Rig->SetSkinnedAssetAndUpdate(Mesh);

	// Keep the reference pose to build each frame's pose from.
	const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
	RigRefLocal = Skeleton.GetRefBonePose();
	RigRefComponent.SetNum(RigRefLocal.Num());
	for (int32 Bone = 0; Bone < RigRefLocal.Num(); ++Bone)
	{
		const int32 Parent = Skeleton.GetParentIndex(Bone);
		RigRefComponent[Bone] = (Parent == INDEX_NONE ? FQuat::Identity : RigRefComponent[Parent]) * RigRefLocal[Bone].GetRotation();
	}
	Rig->SetRelativeScale3D(FVector(Scale));
	Rig->SetRelativeRotation(Rotation);
	Rig->SetRelativeLocation(ModelOffset);
	for (int32 Slot = 0; Slot < Rig->GetNumMaterials(); ++Slot)
	{
		if (UMaterialInstanceDynamic* Material = Rig->CreateDynamicMaterialInstance(Slot))
		{
			Material->SetScalarParameterValue(LWNeon::GlowParam, ModelGlow);
		}
	}
}

void ALightWalkerPawn::UpdateModelStride(bool bMoving, float DeltaSeconds)
{
	// The whole model bobs with each stride and leans into the run, easing in and out; the rig also runs (AnimateRig).
	// While gliding the stride fades out and only a smooth lean is left.
	StrideBlend = FMath::FInterpTo(StrideBlend, bMoving ? 1.f : 0.f, DeltaSeconds, 8.f);
	StrideTime += DeltaSeconds * 11.f * StrideBlend * (1.f - GlideBlend);
	const float Stride = FMath::Sin(StrideTime) * (1.f - GlideBlend);
	const float LeanDegrees = FMath::Lerp(3.f, 9.f, GlideBlend);
	const FQuat Lean = FRotator(-LeanDegrees * StrideBlend, 0.f, Stride * 3.f * StrideBlend).Quaternion();
	const FVector Feet(0.f, 0.f, -ContactRadius);

	Rig->SetRelativeRotation(Lean * FRotator(0.f, ModelYaw, 0.f).Quaternion());
	Rig->SetRelativeLocation(Feet + Lean.RotateVector(ModelOffset - Feet) + FVector(0.f, 0.f, FMath::Abs(Stride) * 5.f * StrideBlend));
	AnimateRig(DeltaSeconds);
}

void ALightWalkerPawn::AnimateRig(float DeltaSeconds)
{
	USkinnedAsset* Asset = Rig->GetSkinnedAsset();
	if (!Asset || Rig->BoneSpaceTransforms.Num() != RigRefLocal.Num())
	{
		return;
	}
	IdleTime += DeltaSeconds;
	const FReferenceSkeleton& Skeleton = Asset->GetRefSkeleton();
	TArray<FTransform>& Pose = Rig->BoneSpaceTransforms;
	Pose = RigRefLocal;

	// Turns a bone about an axis given in the model's own space (+X the hero's left, +Y forward, +Z up), carried
	// along by any turn of its parents. About +X, a positive angle swings a hanging limb forward.
	auto Turn = [&](const TCHAR* BoneName, const FVector& Axis, float Degrees)
	{
		const int32 Bone = Skeleton.FindBoneIndex(FName(BoneName));
		if (Bone == INDEX_NONE || FMath::IsNearlyZero(Degrees))
		{
			return;
		}
		const FQuat Delta(Axis, FMath::DegreesToRadians(Degrees));
		const FQuat& Reference = RigRefComponent[Bone];
		Pose[Bone].SetRotation(Pose[Bone].GetRotation() * (Reference.Inverse() * Delta * Reference));
	};
	const FVector Side(1.f, 0.f, 0.f);
	const FVector Up(0.f, 0.f, 1.f);

	// The run: legs swing opposite each other, each knee bending as its leg comes through, the free arm swinging
	// against its leg, the hips and shoulders twisting against each other. The staff arm only bobs.
	const float Run = StrideBlend * (1.f - GlideBlend) * (1.f - AirBlend);
	const float S = FMath::Sin(StrideTime);
	const float C = FMath::Cos(StrideTime);
	Turn(TEXT("thigh_l"), Side, RunLegSwing * S * Run);
	Turn(TEXT("thigh_r"), Side, -RunLegSwing * S * Run);
	Turn(TEXT("calf_l"), Side, -RunKneeBend * (0.2f + 0.8f * FMath::Max(0.f, C)) * Run);
	Turn(TEXT("calf_r"), Side, -RunKneeBend * (0.2f + 0.8f * FMath::Max(0.f, -C)) * Run);
	Turn(TEXT("foot_l"), Side, -0.3f * RunLegSwing * S * Run);
	Turn(TEXT("foot_r"), Side, 0.3f * RunLegSwing * S * Run);
	Turn(TEXT("pelvis"), Up, 6.f * S * Run);
	Turn(TEXT("spine_01"), Side, -6.f * Run);
	Turn(TEXT("spine_02"), Up, -9.f * S * Run);
	Turn(TEXT("head"), Up, 4.f * S * Run);
	Turn(TEXT("upperarm_l"), Side, -RunArmSwing * S * Run + 6.f * Run);
	Turn(TEXT("lowerarm_l"), Side, (14.f + 8.f * FMath::Max(0.f, -S)) * Run);
	Turn(TEXT("upperarm_r"), Side, 4.f * S * Run);

	// Standing: slow breathing and the odd glance about.
	const float Still = (1.f - StrideBlend) * (1.f - AirBlend);
	Turn(TEXT("spine_02"), Side, 1.5f * FMath::Sin(IdleTime * 2.2f) * Still);
	Turn(TEXT("head"), Up, 6.f * FMath::Sin(IdleTime * 0.45f) * Still);
	Turn(TEXT("upperarm_r"), Side, 1.5f * FMath::Sin(IdleTime * 2.2f + 0.6f) * Still);

	// Gliding on the light orb: legs together and trailing, the free arm swept back.
	Turn(TEXT("thigh_l"), Side, -14.f * GlideBlend);
	Turn(TEXT("thigh_r"), Side, -10.f * GlideBlend);
	Turn(TEXT("calf_l"), Side, -22.f * GlideBlend);
	Turn(TEXT("calf_r"), Side, -16.f * GlideBlend);
	Turn(TEXT("upperarm_l"), Side, -22.f * GlideBlend);

	// Jumping (The Path): the leading knee drives up on the way up, and the legs reach for the ground on the way down.
	const float Rising = AirBlend * FMath::Clamp(0.5f + 0.5f * AirRise, 0.f, 1.f);
	const float Falling = AirBlend - Rising;
	Turn(TEXT("thigh_l"), Side, 30.f * Rising + 16.f * Falling);
	Turn(TEXT("calf_l"), Side, -44.f * Rising - 12.f * Falling);
	Turn(TEXT("thigh_r"), Side, -18.f * Rising - 6.f * Falling);
	Turn(TEXT("calf_r"), Side, -22.f * Rising - 14.f * Falling);
	Turn(TEXT("upperarm_l"), Side, 26.f * Rising - 20.f * Falling);
	Turn(TEXT("lowerarm_l"), Side, 16.f * AirBlend);
	Turn(TEXT("spine_01"), Side, -5.f * Rising + 3.f * Falling);

	Rig->MarkRefreshTransformDirty();
}

bool ALightWalkerPawn::IsVulnerable() const
{
	return !bDowned && GraceRemaining <= 0.f && !IsDevAutoRun();
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

	if (bFrozen)
	{
		UpdateModelStride(false, DeltaSeconds);
		return;
	}

	const FRunUpgrades Upgrades = GetRun() ? GetRun()->GetUpgrades() : FRunUpgrades();

	if (IsDevAutoRun())
	{
		const float Angle = GetWorld()->GetTimeSeconds() * 0.6f;
		MoveInput = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	}

	const FVector Move = UpdateMovement(MoveInput.GetClampedToMaxSize(1.f), DeltaSeconds);
	if (bDowned)
	{
		return;  // fell, or was caught, while moving
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
		FireDirection = AimToWorld(AimInput).GetSafeNormal();
	}
	else if (bCursorAim)
	{
		FireDirection = GetCursorDirection();
	}
	else if (bFireHeld)
	{
		FireDirection = Heading;
	}

	if (!FireDirection.IsNearlyZero())
	{
		FaceDirection(FireDirection);
		if (FireCooldown <= 0.f && ShootsBolts())
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

FVector ALightWalkerPawn::UpdateMovement(const FVector2D& Input, float DeltaSeconds)
{
	const FVector Move = ScreenToWorld(Input);
	if (!Move.IsNearlyZero() && !IsDevAutoRun())
	{
		const float Speed = MoveSpeed * (GetRun() ? GetRun()->GetUpgrades().MoveSpeed : 1.f);
		FVector Location = GetActorLocation() + Move * Speed * DeltaSeconds;
		if (PlayBounds.bIsValid)
		{
			Location.X = FMath::Clamp(Location.X, PlayBounds.Min.X, PlayBounds.Max.X);
			Location.Y = FMath::Clamp(Location.Y, PlayBounds.Min.Y, PlayBounds.Max.Y);
		}
		SetActorLocation(Location);
	}
	return Move;
}

bool ALightWalkerPawn::IsWithin(const FVector& Point, float Distance) const
{
	return FVector::DistSquared2D(Point, GetActorLocation()) <= FMath::Square(Distance);
}

bool ALightWalkerPawn::ProjectCursor(const FVector& Origin, const FVector& Direction, FVector& OutPoint) const
{
	// The arena plane, at the pawn's height.
	const float Distance = FMath::IsNearlyZero(Direction.Z) ? -1.f : (GetActorLocation().Z - Origin.Z) / Direction.Z;
	OutPoint = Origin + Direction * Distance;
	return Distance > 0.f;
}

FVector ALightWalkerPawn::GetMuzzle(const FVector& Direction) const
{
	// The lantern, pulled in line with the centre so close-range shots still land.
	const FVector Lantern2D = HeadingPivot->GetComponentTransform().TransformPosition(FVector(LanternOffset.X, LanternOffset.Y * 0.4f, 0.f));
	return FVector(Lantern2D.X, Lantern2D.Y, GetActorLocation().Z);
}

void ALightWalkerPawn::GiveCarriedShield()
{
	ULightWalkerGameInstance* Run = GetRun();
	if (!Run || !Run->bCarriedShield)
	{
		return;
	}
	Run->bCarriedShield = false;
	bShield = true;
	for (UStaticMeshComponent* Mote : ShieldMotes)
	{
		Mote->SetVisibility(true);
	}
}

void ALightWalkerPawn::UpdatePowerUps(float DeltaSeconds)
{
	bool bExpired = false;
	for (float& Time : PowerTime)
	{
		const bool bWasActive = Time > 0.f;
		Time = FMath::Max(0.f, Time - DeltaSeconds);
		bExpired |= bWasActive && Time <= 0.f;
	}
	if (bExpired)
	{
		ULWSoundSubsystem::Play(this, ELWSound::PowerDown);
	}

	// The light grows with the light orb and with book upgrades, and eases back when the orb runs out.
	const float RunScale = GetRun() ? GetRun()->GetUpgrades().LightRadius : 1.f;
	const float OrbScale = GetPowerUpTimeLeft(EPickupType::LightOrb) > 0.f ? LightOrbScale : 1.f;
	const float LightScale = GetLightScale();
	const float TargetRadius = LightRadius * RunScale * OrbScale * LightScale;
	Glow->SetAttenuationRadius(FMath::FInterpTo(Glow->AttenuationRadius, TargetRadius, DeltaSeconds, 4.f));
	Glow->SetIntensity(30.f * OrbScale * GetLightStrength());

	// The light orb lifts Lightwalker off the ground: rise, hover gently, and settle back when it runs out.
	GlideBlend = FMath::FInterpTo(GlideBlend, OrbScale > 1.f ? 1.f : 0.f, DeltaSeconds, 3.f);
	GlideTime += DeltaSeconds;

	// While gliding, leave a trail of warm light motes behind (only when actually moving).
	const FVector Here = GetActorLocation();
	const bool bMoving = FVector::DistSquared2D(Here, LastTrailLocation) > FMath::Square(60.f * DeltaSeconds);
	LastTrailLocation = Here;
	TrailTimer -= DeltaSeconds;
	if (GlideBlend > 0.3f && bMoving && TrailTimer <= 0.f)
	{
		TrailTimer = 0.018f;
		FLWEmit Mote;
		Mote.Count = 1;
		Mote.Origin = Here + FVector(0.f, 0.f, GlideHeight * GlideBlend - 20.f);
		Mote.OriginRadius = 20.f;
		Mote.Speed = FVector2D(10.f, 40.f);
		// Long-lived and shrinking, so the trail stretches well behind and tapers to a point.
		Mote.Life = FVector2D(1.1f, 1.6f);
		Mote.Size = FVector2D(0.06f, 0.1f);
		Mote.Drag = 2.f;
		Mote.Gravity = -40.f;
		GlideTrail->Emit(Mote);
	}
	HeadingPivot->SetRelativeLocation(FVector(0.f, 0.f, GlideBlend * (GlideHeight + FMath::Sin(GlideTime * 2.5f) * 5.f)));

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

	const bool bOrb = GetPowerUpTimeLeft(EPickupType::LightOrb) > 0.f;
	const bool bPiercing = GetPowerUpTimeLeft(EPickupType::PiercingBeam) > 0.f;
	const bool bSpread = GetPowerUpTimeLeft(EPickupType::SpreadShot) > 0.f;
	ULWSoundSubsystem::Play(this, bPiercing ? ELWSound::ShotPiercing : bSpread ? ELWSound::ShotSpread : ELWSound::Shot);
	ULWFeelSubsystem::Play(this, ELWFeel::Fire);
	const float BoltSpeed = GetRun() ? GetRun()->GetUpgrades().BoltSpeed : 1.f;
	const bool bPowered = bOrb || bPiercing || BoltSpeed > 1.f;

	const FVector Start = GetMuzzle(Direction);

	for (const float Yaw : bSpread ? TArray<float>{ -SpreadAngle, 0.f, SpreadAngle } : TArray<float>{ 0.f })
	{
		const FVector BoltDirection = FQuat(GetFanAxis(), FMath::DegreesToRadians(Yaw)).RotateVector(Direction);
		if (ALaser* Laser = World->SpawnActor<ALaser>(LaserClass, Start, BoltDirection.Rotation(), Params))
		{
			Laser->Launch(BoltDirection, GetShotBounds());
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
	if (Type != EPickupType::Book)
	{
		ULWFeelSubsystem::Play(this, ELWFeel::Pickup);
	}
	switch (Type)
	{
	case EPickupType::LightOrb:
		ULWSoundSubsystem::Play(this, ELWSound::PowerUp);
		ULWSoundSubsystem::Play(this, ELWSound::Glide);
		PowerTime[static_cast<int32>(Type)] = LightOrbDuration;
		ShowPickupMessage(Title, TEXT("Glide on the light: bigger light, bigger bolts, faster firing"));
		break;
	case EPickupType::SpreadShot:
		ULWSoundSubsystem::Play(this, ELWSound::PowerUp);
		PowerTime[static_cast<int32>(Type)] = SpreadShotDuration;
		ShowPickupMessage(Title, TEXT("Three bolts at once"));
		break;
	case EPickupType::PiercingBeam:
		ULWSoundSubsystem::Play(this, ELWSound::PowerUp);
		PowerTime[static_cast<int32>(Type)] = PiercingBeamDuration;
		ShowPickupMessage(Title, TEXT("Bolts pass through everything"));
		break;
	case EPickupType::Shield:
		ULWSoundSubsystem::Play(this, ELWSound::PowerUp);
		bShield = true;
		for (UStaticMeshComponent* Mote : ShieldMotes)
		{
			Mote->SetVisibility(true);
		}
		ShowPickupMessage(Title, TEXT("Blocks the next hit"));
		break;
	case EPickupType::Book:
		ULWSoundSubsystem::Play(this, ELWSound::Book);
		ULWFeelSubsystem::Play(this, ELWFeel::Book);
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
	GlideBlend = 0.f;
	HeadingPivot->SetRelativeLocation(FVector::ZeroVector);
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
