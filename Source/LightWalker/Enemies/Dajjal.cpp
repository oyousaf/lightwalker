#include "Enemies/Dajjal.h"

#include "Effects/LWParticles.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Player/LightWalkerPawn.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Square base half-width, base height and apex height, relative to the actor.
	constexpr float BaseHalf = 42.f;
	constexpr float BaseZ = -30.f;
	constexpr float ApexZ = 52.f;

	/** Point on the front face's centre line, Fraction of the way from the base edge to the apex. */
	FVector FrontFacePoint(float Fraction)
	{
		return FMath::Lerp(FVector(BaseHalf, 0.f, BaseZ), FVector(0.f, 0.f, ApexZ), Fraction);
	}

	FVector Corner(int32 Index)
	{
		static const FVector2D Signs[4] = { { 1.f, 1.f }, { -1.f, 1.f }, { -1.f, -1.f }, { 1.f, -1.f } };
		const FVector2D Sign = Signs[Index % 4];
		return FVector(BaseHalf * Sign.X, BaseHalf * Sign.Y, BaseZ);
	}

	const FVector Apex(0.f, 0.f, ApexZ);

	/** Side face Index runs from Corner(Index) to Corner(Index + 1); face 3 is the front (+X) with the eye. */
	constexpr float FaceIntensity[4] = { 0.8f, 0.3f, 0.5f, 1.3f };
	constexpr int32 BaseSection = 4;

	/** Pitch that tilts +X to point straight out of the front face. */
	float FrontFacePitch()
	{
		return FMath::RadiansToDegrees(FMath::Atan2(BaseHalf, ApexZ - BaseZ));
	}
}

ADajjal::ADajjal()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Radius = 50.f;

	Pyramid = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Pyramid"));
	Pyramid->SetupAttachment(Visual);
	Pyramid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pyramid->SetCastShadow(false);
	Pyramid->bUseAsyncCooking = true;

	EyePivot = CreateDefaultSubobject<USceneComponent>(TEXT("EyePivot"));
	EyePivot->SetupAttachment(Visual);
	EyePivot->SetRelativeLocation(FrontFacePoint(0.36f) + FVector(1.f, 0.f, 0.5f));
	EyePivot->SetRelativeRotation(FRotator(FrontFacePitch(), 0.f, 0.f));
	// Scaled so the eye and its markings stay well inside the face's outline from every angle.
	EyePivot->SetRelativeScale3D(FVector(0.6f));

	// The eye: an almond of white, a gold iris and a black pupil, layered outwards from the face.
	UStaticMeshComponent* EyeWhite = AddShape(TEXT("EyeWhite"), SphereMesh.Object, EyePivot,
		FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.04f, 0.4f, 0.18f));
	UStaticMeshComponent* Iris = AddShape(TEXT("Iris"), SphereMesh.Object, EyePivot,
		FVector(1.5f, 0.f, 0.f), FRotator::ZeroRotator, FVector(0.04f, 0.13f, 0.13f));
	UStaticMeshComponent* Pupil = AddShape(TEXT("Pupil"), SphereMesh.Object, EyePivot,
		FVector(2.5f, 0.f, 0.f), FRotator::ZeroRotator, FVector(0.03f, 0.06f, 0.06f));

	// Horus markings: the brow, the tear line beneath, and the long line from the outer corner.
	UStaticMeshComponent* Brow = AddShape(TEXT("Brow"), CubeMesh.Object, EyePivot,
		FVector(0.f, 0.f, 12.f), FRotator::ZeroRotator, FVector(0.02f, 0.38f, 0.03f));
	UStaticMeshComponent* Tear = AddShape(TEXT("Tear"), CubeMesh.Object, EyePivot,
		FVector(0.f, 3.f, -14.f), FRotator(0.f, 0.f, 8.f), FVector(0.02f, 0.025f, 0.16f));
	UStaticMeshComponent* Tail = AddShape(TEXT("Tail"), CubeMesh.Object, EyePivot,
		FVector(0.f, -24.f, -3.f), FRotator(0.f, 0.f, -12.f), FVector(0.02f, 0.14f, 0.025f));

	// Faces glow at different strengths to fake shading, and bright edges outline the shape.
	for (int32 Face = 0; Face < 4; ++Face)
	{
		AddGlowPart(Pyramid, PyramidColor, FaceIntensity[Face], false, Face);
	}
	AddGlowPart(Pyramid, PyramidColor, 0.2f, false, BaseSection);

	ModelPath = FSoftObjectPath(TEXT("/Game/Enemies/SM_Dajjal.SM_Dajjal"));
	ModelSize = 100.f;
	HitPoints = 3;
	DeathSound = ELWSound::DajjalDeath;
	AddPlaceholder(Pyramid);
	AddPlaceholder(EyePivot);

	for (int32 Index = 0; Index < 4; ++Index)
	{
		UStaticMeshComponent* BaseEdge = AddRod(*FString::Printf(TEXT("BaseEdge%d"), Index), CylinderMesh.Object, Visual,
			Corner(Index), Corner(Index + 1), 3.f);
		UStaticMeshComponent* RidgeEdge = AddRod(*FString::Printf(TEXT("RidgeEdge%d"), Index), CylinderMesh.Object, Visual,
			Corner(Index), Apex, 3.f);
		AddGlowPart(BaseEdge, MarkingColor, 6.f);
		AddGlowPart(RidgeEdge, MarkingColor, 6.f);
		AddPlaceholder(BaseEdge);
		AddPlaceholder(RidgeEdge);
	}
	AddGlowPart(EyeWhite, EyeWhiteColor, 10.f);
	AddGlowPart(Iris, PyramidColor, 14.f);
	AddGlowPart(Pupil, FLinearColor::Black, 0.f, true);
	for (UStaticMeshComponent* Marking : { Brow, Tear, Tail })
	{
		AddGlowPart(Marking, MarkingColor, 10.f);
	}
}

void ADajjal::BeginPlay()
{
	BuildPyramid();
	Super::BeginPlay();
}

void ADajjal::BuildPyramid()
{
	// One section per face, so each can glow at its own strength. Flat-shaded triangles are added with both
	// windings so the faces show from either side.
	auto AddSection = [this](int32 Section, const TArray<FVector>& Points)
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		for (int32 Index = 0; Index + 2 < Points.Num(); Index += 3)
		{
			const FVector& A = Points[Index];
			const FVector& B = Points[Index + 1];
			const FVector& C = Points[Index + 2];
			const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
			const int32 Start = Vertices.Num();
			Vertices.Append({ A, B, C });
			Normals.Append({ Normal, Normal, Normal });
			Triangles.Append({ Start, Start + 1, Start + 2, Start, Start + 2, Start + 1 });
		}
		Pyramid->CreateMeshSection_LinearColor(Section, Vertices, Triangles, Normals, {}, {}, {}, false);
	};

	for (int32 Face = 0; Face < 4; ++Face)
	{
		AddSection(Face, { Corner(Face), Corner(Face + 1), Apex });
	}
	AddSection(BaseSection, { Corner(0), Corner(2), Corner(1), Corner(0), Corner(3), Corner(2) });
}

void ADajjal::InitEnemy(ALightWalkerPawn* InPlayer, const FBox2D& InBounds, float InMoveSpeed)
{
	Super::InitEnemy(InPlayer, InBounds, InMoveSpeed);
	PickDirection();
}

void ADajjal::PickDirection()
{
	static const FVector Directions[] = { FVector::ForwardVector, FVector::BackwardVector, FVector::RightVector, FVector::LeftVector };
	Direction = Directions[FMath::RandRange(0, UE_ARRAY_COUNT(Directions) - 1)];
	RedirectRemaining = FMath::FRandRange(RedirectInterval.X, RedirectInterval.Y);
}

void ADajjal::UpdateMovement(float DeltaSeconds)
{
	RedirectRemaining -= DeltaSeconds;
	if (RedirectRemaining <= 0.f)
	{
		PickDirection();
	}

	FVector Location = GetActorLocation() + Direction * MoveSpeed * DeltaSeconds;

	if (Bounds.bIsValid)
	{
		if (Location.X <= Bounds.Min.X || Location.X >= Bounds.Max.X)
		{
			Direction.X = -Direction.X;
		}
		if (Location.Y <= Bounds.Min.Y || Location.Y >= Bounds.Max.Y)
		{
			Direction.Y = -Direction.Y;
		}
		Location.X = FMath::Clamp(Location.X, Bounds.Min.X, Bounds.Max.X);
		Location.Y = FMath::Clamp(Location.Y, Bounds.Min.Y, Bounds.Max.Y);
	}

	SetActorLocation(Location);
}

void ADajjal::UpdateLook(float DeltaSeconds)
{
	// The whole pyramid turns so its eye keeps watching Lightwalker, even during the wave intro.
	if (const ALightWalkerPawn* LightWalker = GetPlayer())
	{
		TurnVisualTowards(LightWalker->GetActorLocation() - GetActorLocation(), DeltaSeconds, WatchTurnRate);
	}

	Visual->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(LookTime * 2.f) * 5.f));
}

void ADajjal::SpawnDeathEffect()
{
	LWFX::DajjalDeath(GetWorld(), GetActorLocation());
}
