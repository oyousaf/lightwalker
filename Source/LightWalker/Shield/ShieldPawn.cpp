#include "Shield/ShieldPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Core/LightWalkerGameInstance.h"
#include "Core/LWFeel.h"
#include "Core/LWNeon.h"
#include "Core/LWSound.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Shield/ShieldField.h"

namespace
{
	constexpr int32 SegmentCount = 15;
}

AShieldPawn::AShieldPawn()
{
	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		UStaticMeshComponent* Segment = LWNeon::MakeMesh(this, *FString::Printf(TEXT("ShieldSegment%d"), Index), ELWShape::Cube, RootComponent);
		Segments.Add(Segment);
	}
}

void AShieldPawn::BeginPlay()
{
	Super::BeginPlay();
	for (UStaticMeshComponent* Segment : Segments)
	{
		SegmentMaterials.Add(LWNeon::ApplyGlow(Segment, ShieldColor, 3.f));
	}
	UpdateShield(0.f);
}

void AShieldPawn::PlaceAt(const FVector& Location, float GraceSeconds)
{
	Revive(Location, GraceSeconds);
	SlideVelocity = 0.f;
	Tilt = 0.f;
	StrainLeft = 0.f;
	Heading = FVector::ForwardVector;
}

FVector2D AShieldPawn::GetShieldFacing() const
{
	return FVector2D(FMath::Cos(Tilt), FMath::Sin(Tilt));
}

FVector2D AShieldPawn::GetShieldCentre() const
{
	return FVector2D(GetActorLocation()) + GetShieldFacing() * ShieldDistance;
}

void AShieldPawn::StrainShield()
{
	StrainLeft = StrainTime;
}

FVector AShieldPawn::UpdateMovement(const FVector2D& Input, float DeltaSeconds)
{
	FVector2D Wanted = bHeld ? FVector2D::ZeroVector : Input;
#if !UE_BUILD_SHIPPING
	static const bool bAuto = FParse::Param(FCommandLine::Get(), TEXT("LWAutoShield"));
	if (bAuto && !bHeld)
	{
		Wanted = AutoPlay(DeltaSeconds);
	}
#endif

	const float Speed = SlideSpeed * (GetRun() ? GetRun()->GetUpgrades().MoveSpeed : 1.f);
	SlideVelocity = FMath::FInterpConstantTo(SlideVelocity, Wanted.X * Speed, DeltaSeconds, SlideAcceleration);
	FVector Location = GetActorLocation();
	Location.Y += SlideVelocity * DeltaSeconds;
	if (PlayBounds.bIsValid)
	{
		if (Location.Y <= PlayBounds.Min.Y || Location.Y >= PlayBounds.Max.Y)
		{
			SlideVelocity = 0.f;
		}
		Location.Y = FMath::Clamp(Location.Y, PlayBounds.Min.Y, PlayBounds.Max.Y);
	}
	SetActorLocation(Location);

	// Tilt: the right stick, or towards the mouse while it's held; level otherwise.
	float WantedTilt = 0.f;
	if (!bHeld && AimInput.Size() >= AimThreshold)
	{
		WantedTilt = FMath::Atan2(AimInput.X, FMath::Max(AimInput.Y, 0.15f));
	}
	else if (!bHeld && bCursorAim)
	{
		const FVector To = CursorTarget - Location;
		WantedTilt = FMath::Atan2(To.Y, FMath::Max(To.X, 100.f));
	}
	const float Most = FMath::DegreesToRadians(MaxTilt);
	Tilt = FMath::FInterpTo(Tilt, FMath::Clamp(WantedTilt, -Most, Most), DeltaSeconds, 14.f);

	return FMath::Abs(SlideVelocity) > 80.f ? FVector(0.f, FMath::Sign(SlideVelocity), 0.f) : FVector::ZeroVector;
}

void AShieldPawn::FaceDirection(const FVector& Direction)
{
	// Always up the arena, leaning into the slide and the tilt.
	const float Lean = FMath::Clamp(SlideVelocity / SlideSpeed, -1.f, 1.f) * 38.f;
	const FRotator Wanted(0.f, FMath::RadiansToDegrees(Tilt) * 0.7f + Lean, 0.f);
	HeadingPivot->SetWorldRotation(FMath::RInterpTo(HeadingPivot->GetComponentRotation(), Wanted, GetWorld()->GetDeltaSeconds(), 12.f));
}

void AShieldPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsDowned())
	{
		UpdateShield(DeltaSeconds);
	}
}

void AShieldPawn::UpdateShield(float DeltaSeconds)
{
	StrainLeft = FMath::Max(0.f, StrainLeft - DeltaSeconds);
	ShieldFlash = FMath::Max(0.f, ShieldFlash - DeltaSeconds * 4.f);
	const bool bWide = GetPowerUpTimeLeft(EPickupType::LightOrb) > 0.f;
	const float Wanted = ShieldHalfWidth * (bWide ? WideScale : 1.f) * (StrainLeft > 0.f ? StrainScale : 1.f);
	ShownHalfWidth = DeltaSeconds > 0.f ? FMath::FInterpTo(ShownHalfWidth, Wanted, DeltaSeconds, 8.f) : Wanted;

	const FVector2D Facing = GetShieldFacing();
	const FVector2D Along(-Facing.Y, Facing.X);
	const FVector2D Centre = GetShieldCentre();
	const float Length = ShownHalfWidth * 2.f / SegmentCount;
	const float Flicker = StrainLeft > 0.f ? 0.75f + 0.25f * FMath::Sin(StrainLeft * 40.f) : 1.f;
	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		const float U = (Index + 0.5f) / SegmentCount * 2.f - 1.f;
		const FVector2D At = Centre + Along * (U * ShownHalfWidth) - Facing * (U * U * ShieldBend);
		const float Bend = FMath::Atan2(2.f * U * ShieldBend, ShownHalfWidth);
		Segments[Index]->SetWorldLocation(FVector(At.X, At.Y, GetActorLocation().Z));
		Segments[Index]->SetWorldRotation(FRotator(0.f, FMath::RadiansToDegrees(Tilt + Bend), 0.f));
		Segments[Index]->SetWorldScale3D(FVector(0.12f, Length * 1.08f / 100.f, 0.5f));
		if (SegmentMaterials.IsValidIndex(Index) && SegmentMaterials[Index])
		{
			SegmentMaterials[Index]->SetVectorParameterValue(LWNeon::ColorParam, StrainLeft > 0.f ? FLinearColor(1.f, 0.25f, 0.4f) : ShieldColor);
			SegmentMaterials[Index]->SetScalarParameterValue(LWNeon::IntensityParam, (1.8f + 7.f * ShieldFlash * ShieldFlash) * Flicker);
		}
	}
}

void AShieldPawn::CollectPickup(EPickupType Type)
{
	// The same power-ups as the arena, doing this mode's job.
	switch (Type)
	{
	case EPickupType::LightOrb:
		Super::CollectPickup(Type);
		ShowPickupMessage(LWPickups::GetName(Type), TEXT("A wider shield"));
		break;
	case EPickupType::SpreadShot:
		ULWFeelSubsystem::Play(this, ELWFeel::Pickup);
		ULWSoundSubsystem::Play(this, ELWSound::PowerUp);
		ShowPickupMessage(TEXT("SPLIT BEAM"), TEXT("Every beam becomes three"));
		break;
	case EPickupType::PiercingBeam:
		Super::CollectPickup(Type);
		ShowPickupMessage(LWPickups::GetName(Type), TEXT("The beam burns straight through"));
		break;
	case EPickupType::Shield:
		Super::CollectPickup(Type);
		ShowPickupMessage(LWPickups::GetName(Type), TEXT("Saves one lost beam, or one hit"));
		break;
	default:
		Super::CollectPickup(Type);
		break;
	}
	if (OnPickup)
	{
		OnPickup(Type);
	}
}

#if !UE_BUILD_SHIPPING
FVector2D AShieldPawn::AutoPlay(float DeltaSeconds)
{
	AutoTimer -= DeltaSeconds;
	if (AutoTimer <= 0.f)
	{
		// Meets the beam off-centre by a changing amount, so it sweeps the whole wall.
		AutoTimer = FMath::FRandRange(0.8f, 1.8f);
		AutoOffset = FMath::FRandRange(-0.5f, 0.5f);
	}
	bAutoRelease = true;
	float Wanted = 0.f;
	if (!Field || !Field->PredictLanding(GetShieldCentre().X, Wanted))
	{
		return FVector2D::ZeroVector;
	}
	const float Gap = Wanted - AutoOffset * ShownHalfWidth - GetActorLocation().Y;
	return FVector2D(FMath::Clamp(Gap / 45.f, -1.f, 1.f), 0.f);
}
#endif
