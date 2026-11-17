#include "Arena/Arena.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float EdgeThickness = 24.f;
	constexpr float EdgeHeight = 40.f;
	constexpr float FloorMargin = 2000.f;

	UStaticMeshComponent* MakeMesh(AActor* Owner, const TCHAR* Name, UStaticMesh* Mesh, USceneComponent* Parent)
	{
		UStaticMeshComponent* Component = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(Parent);
		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		return Component;
	}
}

AArena::AArena()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Floor = MakeMesh(this, TEXT("Floor"), PlaneMesh.Object, RootComponent);
	EdgeTop = MakeMesh(this, TEXT("EdgeTop"), CubeMesh.Object, RootComponent);
	EdgeBottom = MakeMesh(this, TEXT("EdgeBottom"), CubeMesh.Object, RootComponent);
	EdgeLeft = MakeMesh(this, TEXT("EdgeLeft"), CubeMesh.Object, RootComponent);
	EdgeRight = MakeMesh(this, TEXT("EdgeRight"), CubeMesh.Object, RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->bConstrainAspectRatio = false;

	Moonlight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moonlight"));
	Moonlight->SetupAttachment(RootComponent);
	Moonlight->SetRelativeRotation(FRotator(-60.f, 30.f, 0.f));
	Moonlight->SetIntensity(1.5f);
	Moonlight->SetLightColor(FLinearColor(0.45f, 0.5f, 1.f));
	Moonlight->SetCastShadows(false);

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;
}

void AArena::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Layout();
	ApplyPostProcess();
}

void AArena::BeginPlay()
{
	Super::BeginPlay();

	LWNeon::ApplySurface(Floor, FloorColor);
	for (UStaticMeshComponent* Edge : { EdgeTop.Get(), EdgeBottom.Get(), EdgeLeft.Get(), EdgeRight.Get() })
	{
		LWNeon::ApplyGlow(Edge, EdgeColor, EdgeGlowIntensity);
	}
}

void AArena::Layout()
{
	// Engine basic shapes are 100 units across.
	const FVector2D Full = HalfExtent * 2.f;

	Floor->SetRelativeScale3D(FVector((Full.X + FloorMargin) / 100.f, (Full.Y + FloorMargin) / 100.f, 1.f));

	const float EdgeZ = EdgeHeight * 0.5f;
	const float SpanX = (Full.X + EdgeThickness * 2.f) / 100.f;
	const float SpanY = (Full.Y + EdgeThickness * 2.f) / 100.f;
	const float Thin = EdgeThickness / 100.f;
	const float Tall = EdgeHeight / 100.f;

	EdgeTop->SetRelativeLocation(FVector(HalfExtent.X + EdgeThickness * 0.5f, 0.f, EdgeZ));
	EdgeTop->SetRelativeScale3D(FVector(Thin, SpanY, Tall));
	EdgeBottom->SetRelativeLocation(FVector(-HalfExtent.X - EdgeThickness * 0.5f, 0.f, EdgeZ));
	EdgeBottom->SetRelativeScale3D(FVector(Thin, SpanY, Tall));
	EdgeRight->SetRelativeLocation(FVector(0.f, HalfExtent.Y + EdgeThickness * 0.5f, EdgeZ));
	EdgeRight->SetRelativeScale3D(FVector(SpanX, Thin, Tall));
	EdgeLeft->SetRelativeLocation(FVector(0.f, -HalfExtent.Y - EdgeThickness * 0.5f, EdgeZ));
	EdgeLeft->SetRelativeScale3D(FVector(SpanX, Thin, Tall));

	const FRotator CameraRotation(CameraPitch, 0.f, 0.f);
	Camera->SetRelativeRotation(CameraRotation);
	Camera->SetRelativeLocation(-CameraRotation.Vector() * CameraDistance);
	Camera->SetFieldOfView(CameraFieldOfView);
}

void AArena::ApplyPostProcess()
{
	FPostProcessSettings& Settings = PostProcess->Settings;

	Settings.bOverride_AutoExposureMethod = true;
	Settings.AutoExposureMethod = AEM_Manual;
	Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Settings.AutoExposureApplyPhysicalCameraExposure = false;
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = ExposureCompensation;

	Settings.bOverride_BloomIntensity = true;
	Settings.BloomIntensity = BloomIntensity;

	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = 0.5f;

	// Motion blur smears the spinning, overlapping glow shapes into blobs.
	Settings.bOverride_MotionBlurAmount = true;
	Settings.MotionBlurAmount = 0.f;
}

FBox2D AArena::GetPlayBounds(float Radius) const
{
	const FVector Centre = GetCentre();
	const FVector2D Inner = FVector2D(
		FMath::Max(0.f, HalfExtent.X - Radius),
		FMath::Max(0.f, HalfExtent.Y - Radius));
	return FBox2D(FVector2D(Centre.X, Centre.Y) - Inner, FVector2D(Centre.X, Centre.Y) + Inner);
}
