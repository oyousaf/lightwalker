#include "Effects/LWWeather.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Effects/LWParticles.h"
#include "Kismet/GameplayStatics.h"

ALWWeather::ALWWeather()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ALWWeather::Setup(const FBox& InAir, bool bInFollowCamera)
{
	Air = InAir;
	bFollowCamera = bInFollowCamera;
	Refresh();
}

void ALWWeather::Refresh()
{
	const ELWSeason Season = LWSeason::Current();
	if (Season == Shown)
	{
		return;
	}
	Shown = Season;
	for (UInstancedStaticMeshComponent* Mesh : Meshes)
	{
		Mesh->DestroyComponent();
	}
	Meshes.Reset();
	Layers.Reset();

	const FVector Extent = Air.GetSize();
	for (const FLWWeatherLayer& Look : LWSeason::GetWeather())
	{
		FLayer& Layer = Layers.AddDefaulted_GetRef();
		Layer.Look = Look;
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Look.Count * ULWParticleComponent::GetQualityScale()));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FThing& Thing = Layer.Things.AddDefaulted_GetRef();
			Thing.Position = FVector(FMath::FRand() * Extent.X, FMath::FRand() * Extent.Y, FMath::FRand() * Extent.Z);
			Thing.Fall = FMath::FRandRange(Look.Fall.X, Look.Fall.Y);
			Thing.Size = FMath::FRandRange(Look.Size.X, Look.Size.Y);
			Thing.Phase = FMath::FRand() * UE_TWO_PI;
		}
		Layer.Transforms.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(0.001f)), Count);

		const ELWShape Shape = Look.Kind == FLWWeatherLayer::EKind::Streak ? ELWShape::Cube : ELWShape::Sphere;
		UInstancedStaticMeshComponent* Mesh = LWNeon::AddInstances(this, Shape, RootComponent);
		Mesh->AddInstances(Layer.Transforms, false, true);
		LWNeon::ApplyGlow(Mesh, Look.Color, Look.Glow);
		Meshes.Add(Mesh);
	}
}

void ALWWeather::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	FVector Corner = Air.Min;
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector Eye = Camera ? Camera->GetCameraLocation() : FVector(0.f, 0.f, 100000.f);
	if (bFollowCamera && Camera)
	{
		// Travels along the course; what's in the air stays where it is and wraps round as the box passes.
		Corner += FVector(0.f, Eye.Y, Eye.Z);
	}
	const FVector Extent = Air.GetSize();
	auto Wrap = [](double Value, double From, double Length) { return From + FMath::Fmod(FMath::Fmod(Value - From, Length) + Length, Length); };

	for (int32 Index = 0; Index < Layers.Num(); ++Index)
	{
		FLayer& Layer = Layers[Index];
		const FLWWeatherLayer& Look = Layer.Look;
		// Rain leans the way it's blown.
		const FQuat Lean = FRotator(0.f, 0.f, FMath::RadiansToDegrees(FMath::Atan2(Look.Wind, FMath::Max(1.f, static_cast<float>(Look.Fall.X))))).Quaternion();
		for (int32 Number = 0; Number < Layer.Things.Num(); ++Number)
		{
			FThing& Thing = Layer.Things[Number];
			Thing.Position.Y += Look.Wind * DeltaSeconds;
			Thing.Position.Z -= Thing.Fall * DeltaSeconds;
			Thing.Position.X = Wrap(Thing.Position.X, Corner.X, Extent.X);
			Thing.Position.Y = Wrap(Thing.Position.Y, Corner.Y, Extent.Y);
			Thing.Position.Z = Wrap(Thing.Position.Z, Corner.Z, Extent.Z);

			const float Swing = Time * 1.3f + Thing.Phase;
			const FVector At = Thing.Position + FVector(FMath::Cos(Swing * 0.7f), FMath::Sin(Swing), 0.f) * Look.Sway;
			const float Scale = Thing.Size / 100.f;
			// Grows in and shrinks away at the top and bottom, so nothing pops, and stays out of the camera's face.
			const double Through = (Thing.Position.Z - Corner.Z) / Extent.Z;
			const float Near = FMath::Clamp(static_cast<float>(FVector::Dist(At, Eye) - 700.0) / 500.f, 0.f, 1.f);
			const float Edge = FMath::Max(0.01f, Near * FMath::Clamp(static_cast<float>(FMath::Min(Through, 1.0 - Through)) * 8.f, 0.f, 1.f));
			FTransform& Transform = Layer.Transforms[Number];
			switch (Look.Kind)
			{
			case FLWWeatherLayer::EKind::Streak:
				Transform = FTransform(Lean, At, FVector(Scale, Scale, 0.7f) * Edge);
				break;
			case FLWWeatherLayer::EKind::Scrap:
				Transform = FTransform(FRotator(Swing * 70.f, Swing * 50.f, Swing * 90.f), At, FVector(1.f, 0.6f, 0.12f) * Scale * Edge);
				break;
			case FLWWeatherLayer::EKind::Ember:
				Transform = FTransform(FQuat::Identity, At, FVector(Scale * Edge * (0.75f + 0.25f * FMath::Sin(Swing * 3.f))));
				break;
			default:
				Transform = FTransform(FQuat::Identity, At, FVector(Scale * Edge));
				break;
			}
		}
		Meshes[Index]->BatchUpdateInstancesTransforms(0, Layer.Transforms, true, true, true);
	}
}
