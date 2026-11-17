#include "Enemies/Shaytaan.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Player/LightWalkerPawn.h"
#include "UObject/ConstructorHelpers.h"

AShaytaan::AShaytaan()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Body->SetStaticMesh(CubeMesh.Object);
	Body->SetRelativeScale3D(FVector(0.6f));
	Body->SetRelativeRotation(FRotator(0.f, 45.f, 0.f));

	Radius = 50.f;
	Color = FLinearColor(1.f, 0.05f, 0.35f);
	GlowIntensity = 5.f;
	SpinRate = 120.f;
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
	const float Step = MoveSpeed * DeltaSeconds;
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	SetActorLocation(Location + ToTarget * (FMath::Min(Step, Distance) / Distance));
}
