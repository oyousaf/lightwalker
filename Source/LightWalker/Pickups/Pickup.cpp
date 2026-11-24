#include "Pickups/Pickup.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Effects/LWParticles.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Player/LightWalkerPawn.h"
#include "UI/LWStyle.h"

FString LWPickups::GetName(EPickupType Type)
{
	switch (Type)
	{
	case EPickupType::LightOrb:     return TEXT("LIGHT ORB");
	case EPickupType::SpreadShot:   return TEXT("SPREAD SHOT");
	case EPickupType::PiercingBeam: return TEXT("PIERCING BEAM");
	case EPickupType::Shield:       return TEXT("SHIELD");
	case EPickupType::Book:         return TEXT("BOOK OF KNOWLEDGE");
	default:                        return FString();
	}
}

FLinearColor LWPickups::GetColor(EPickupType Type)
{
	switch (Type)
	{
	case EPickupType::LightOrb:     return FLinearColor(1.f, 0.9f, 0.55f);
	case EPickupType::SpreadShot:   return FLinearColor(1.f, 0.3f, 0.9f);
	case EPickupType::PiercingBeam: return FLinearColor(0.45f, 1.f, 1.f);
	case EPickupType::Shield:       return FLinearColor(0.3f, 1.f, 0.45f);
	case EPickupType::Book:         return FLinearColor(1.f, 0.7f, 0.15f);
	default:                        return FLinearColor::White;
	}
}

UTexture2D* LWPickups::GetIcon(EPickupType Type)
{
	switch (Type)
	{
	case EPickupType::LightOrb:     return LWStyle::Art(TEXT("IconLightOrb"));
	case EPickupType::SpreadShot:   return LWStyle::Art(TEXT("IconSpreadShot"));
	case EPickupType::PiercingBeam: return LWStyle::Art(TEXT("IconPiercingBeam"));
	case EPickupType::Shield:       return LWStyle::Art(TEXT("IconShield"));
	case EPickupType::Book:         return LWStyle::Art(TEXT("IconBook"));
	default:                        return nullptr;
	}
}

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Root);

	Sparkles = CreateDefaultSubobject<ULWParticleComponent>(TEXT("Sparkles"));
	Sparkles->SetupAttachment(Root);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(12.f);
	Light->SetAttenuationRadius(300.f);
	Light->SetCastShadows(false);
	// Kept out of Lumen's global illumination: the blink before a pickup vanishes otherwise leaves dark ghosts.
	Light->SetIndirectLightingIntensity(0.f);
}

void APickup::InitPickup(EPickupType InType, ALightWalkerPawn* InPlayer)
{
	Type = InType;
	Player = InPlayer;

	const FLinearColor Color = LWPickups::GetColor(Type);
	Sparkles->Setup(ELWShape::Sphere, Color, 10.f);
	Light->SetLightColor(Color);

	if (UTexture2D* Icon = LWPickups::GetIcon(Type))
	{
		LWNeon::AddCard(this, Icon, Visual, FVector::ZeroVector, FVector2D(IconSize), FRotator::ZeroRotator, IconGlow);
	}
	else
	{
		LWNeon::ApplyGlow(LWNeon::AddMesh(this, ELWShape::Sphere, Visual, FVector::ZeroVector, FVector(0.5f)), Color, 8.f);
	}
	if (Type == EPickupType::Book)
	{
		LightIntensity = 30.f;
		Light->SetIntensity(LightIntensity);
		Light->SetAttenuationRadius(500.f);
		Lifetime = 14.f;
	}
}

void APickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	AddActorWorldOffset(Drift * DeltaSeconds);
	if (!bPersistent && Age >= Lifetime)
	{
		Destroy();
		return;
	}

	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Visual->SetWorldRotation(LWNeon::CardFacing(Camera->GetCameraRotation() + FRotator(0.f, 0.f, FMath::Sin(Age * 2.1f) * 7.f)));
	}
	Visual->SetRelativeLocation(FVector(0.f, 0.f, 24.f + FMath::Sin(Age * 3.f) * 8.f));

	// Pulse faster and faster as it's about to vanish. A smooth pulse rather than a hard on/off blink: flicking the
	// light and glow off every few frames leaves dark smears in the temporal lighting and anti-aliasing.
	const float Left = bPersistent ? BlinkTime : Lifetime - Age;
	float Pulse = 1.f;
	if (Left < BlinkTime)
	{
		BlinkPhase += DeltaSeconds * FMath::Lerp(14.f, 6.f, Left / BlinkTime);
		Pulse = 0.6f + 0.4f * FMath::Cos(BlinkPhase);
	}
	const bool bVisible = Pulse > 0.5f;
	Visual->SetRelativeScale3D(FVector(Pulse * (1.f + 0.05f * FMath::Sin(Age * 4.f))));

	SparkleTimer -= DeltaSeconds;
	if (SparkleTimer <= 0.f && bVisible)
	{
		SparkleTimer = Type == EPickupType::Book ? 0.06f : 0.14f;
		FLWEmit Mote;
		Mote.Count = 1;
		Mote.Origin = GetActorLocation();
		Mote.OriginRadius = 45.f;
		Mote.Direction = FVector::UpVector;
		Mote.Spread = 10.f;
		Mote.Speed = FVector2D(50.f, 110.f);
		Mote.Life = FVector2D(0.6f, 1.f);
		Mote.Size = FVector2D(0.025f, 0.045f);
		Mote.Drag = 0.5f;
		Sparkles->Emit(Mote);
	}
	Light->SetIntensity(LightIntensity * Pulse * Pulse);

	ALightWalkerPawn* LightWalker = Player.Get();
	if (LightWalker && !LightWalker->IsDowned() && LightWalker->IsWithin(GetActorLocation(), CollectRadius))
	{
		LWFX::PickupCollect(GetWorld(), GetActorLocation(), LWPickups::GetColor(Type), Type == EPickupType::Book);
		LightWalker->CollectPickup(Type);
		Destroy();
	}
}
