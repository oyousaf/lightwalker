#include "Enemies/Dajjal.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADajjal::ADajjal()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Body->SetStaticMesh(CylinderMesh.Object);
	Body->SetRelativeScale3D(FVector(0.95f, 0.95f, 0.25f));

	Radius = 50.f;
	Color = FLinearColor(1.f, 0.55f, 0.02f);
	GlowIntensity = 5.f;
	SpinRate = -90.f;
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
