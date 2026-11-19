#include "Pickups/Pickup.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Engine/StaticMesh.h"
#include "Player/LightWalkerPawn.h"
#include "UObject/ConstructorHelpers.h"

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

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Root);
	Visual->SetRelativeScale3D(FVector(1.5f));

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(12.f);
	Light->SetAttenuationRadius(300.f);
	Light->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	SphereMesh = Sphere.Object;
	CubeMesh = Cube.Object;
	ConeMesh = Cone.Object;
	CylinderMesh = Cylinder.Object;
}

UStaticMeshComponent* APickup::AddPart(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale,
	const FLinearColor& Color, float Intensity)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetupAttachment(Visual);
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCastShadow(false);
	Part->RegisterComponent();
	LWNeon::ApplyGlow(Part, Color, Intensity);
	Parts.Add(Part);
	return Part;
}

void APickup::InitPickup(EPickupType InType, ALightWalkerPawn* InPlayer)
{
	Type = InType;
	Player = InPlayer;

	const FLinearColor Color = LWPickups::GetColor(Type);
	Light->SetLightColor(Color);

	// Basic shapes are 100 units across, centred on their pivot (the cone and cylinder run along +Z).
	switch (Type)
	{
	case EPickupType::LightOrb:
		AddPart(SphereMesh, FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.38f), Color, 9.f);
		AddPart(SphereMesh, FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.2f), FLinearColor::White, 20.f);
		break;

	case EPickupType::SpreadShot:
		for (const float Yaw : { -28.f, 0.f, 28.f })
		{
			const FRotator Fan(0.f, Yaw, 0.f);
			AddPart(ConeMesh, Fan.RotateVector(FVector(12.f, 0.f, 0.f)), FRotator(-90.f, Yaw, 0.f), FVector(0.14f, 0.14f, 0.4f), Color, 8.f);
		}
		break;

	case EPickupType::PiercingBeam:
		AddPart(CylinderMesh, FVector(-6.f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f), FVector(0.07f, 0.07f, 0.55f), Color, 10.f);
		AddPart(ConeMesh, FVector(28.f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f), FVector(0.16f, 0.16f, 0.2f), FLinearColor::White, 14.f);
		break;

	case EPickupType::Shield:
		// Tipped back towards the camera so the disc reads as a disc from above.
		AddPart(CylinderMesh, FVector::ZeroVector, FRotator(0.f, 0.f, 35.f), FVector(0.5f, 0.5f, 0.05f), Color, 6.f);
		AddPart(SphereMesh, FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.16f), FLinearColor::White, 12.f);
		break;

	case EPickupType::Book:
		// An open book: two covers in a shallow V with glowing pages between them. No real script or symbols;
		// the writing is left to the model in milestone 3.
		AddPart(CubeMesh, FVector(0.f, -16.f, 0.f), FRotator(0.f, 0.f, -14.f), FVector(0.42f, 0.32f, 0.04f), Color, 5.f);
		AddPart(CubeMesh, FVector(0.f, 16.f, 0.f), FRotator(0.f, 0.f, 14.f), FVector(0.42f, 0.32f, 0.04f), Color, 5.f);
		AddPart(CubeMesh, FVector(0.f, -15.f, 3.f), FRotator(0.f, 0.f, -14.f), FVector(0.38f, 0.28f, 0.02f), FLinearColor(1.f, 0.95f, 0.8f), 14.f);
		AddPart(CubeMesh, FVector(0.f, 15.f, 3.f), FRotator(0.f, 0.f, 14.f), FVector(0.38f, 0.28f, 0.02f), FLinearColor(1.f, 0.95f, 0.8f), 14.f);
		Light->SetIntensity(30.f);
		Light->SetAttenuationRadius(500.f);
		Lifetime = 14.f;
		break;

	default:
		break;
	}
}

void APickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	if (Age >= Lifetime)
	{
		Destroy();
		return;
	}

	Visual->AddLocalRotation(FRotator(0.f, SpinRate * DeltaSeconds, 0.f));
	Visual->SetRelativeLocation(FVector(0.f, 0.f, 20.f + FMath::Sin(Age * 3.f) * 8.f));

	// Blink faster and faster as it's about to vanish.
	const float Left = Lifetime - Age;
	const bool bVisible = Left > BlinkTime || FMath::Frac(Age * FMath::Lerp(10.f, 4.f, Left / BlinkTime)) > 0.35f;
	Visual->SetVisibility(bVisible, true);
	Light->SetVisibility(bVisible);

	ALightWalkerPawn* LightWalker = Player.Get();
	if (LightWalker && !LightWalker->IsDowned() &&
		FVector::DistSquared2D(GetActorLocation(), LightWalker->GetActorLocation()) <= FMath::Square(CollectRadius))
	{
		LightWalker->CollectPickup(Type);
		Destroy();
	}
}
