#include "Arena/Arena.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float EdgeThickness = 24.f;
	constexpr float EdgeHeight = 40.f;
	// The floor runs well past the arena on every side, so even a 32:9 view never sees its edge.
	constexpr float FloorMargin = 30000.f;

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
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Floor = MakeMesh(this, TEXT("Floor"), PlaneMesh.Object, RootComponent);

	Grid = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Grid"));
	Grid->SetupAttachment(RootComponent);
	Grid->SetStaticMesh(CubeMesh.Object);
	Grid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Grid->SetCastShadow(false);
	// Glow only: kept out of Lumen and distance-field lighting.
	Grid->bAffectDynamicIndirectLighting = false;
	Grid->bAffectDistanceFieldLighting = false;
	Grid->bVisibleInRayTracing = false;
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
	Moonlight->SetIntensity(MoonlightIntensity);
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
	EdgeMaterials.Reset();
	for (UStaticMeshComponent* Edge : { EdgeTop.Get(), EdgeBottom.Get() })
	{
		EdgeMaterials.Add(LWNeon::ApplyGlow(Edge, EdgeColor, EdgeGlowIntensity));
	}
	for (UStaticMeshComponent* Edge : { EdgeLeft.Get(), EdgeRight.Get() })
	{
		EdgeMaterials.Add(LWNeon::ApplyGlow(Edge, SideEdgeColor, EdgeGlowIntensity));
	}
	GridMaterial = LWNeon::ApplyGlow(Grid, GridColor, GridGlowIntensity);
	ApplyCamera();
}

void AArena::Layout()
{
	// Engine basic shapes are 100 units across.
	const FVector2D Full = HalfExtent * 2.f;

	Floor->SetRelativeScale3D(FVector((Full.X + FloorMargin) / 100.f, (Full.Y + FloorMargin) / 100.f, 1.f));

	// Grid lines inside the edges, centred so the pattern is symmetrical. Basic cube is 100 units.
	Grid->ClearInstances();
	constexpr float LineWidth = 3.f;
	constexpr float LineHeight = 0.6f;
	const int32 RowsX = FMath::FloorToInt(HalfExtent.X / GridSpacing);
	const int32 RowsY = FMath::FloorToInt(HalfExtent.Y / GridSpacing);
	for (int32 Index = -RowsX; Index <= RowsX; ++Index)
	{
		Grid->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Index * GridSpacing, 0.f, 0.5f),
			FVector(LineWidth / 100.f, Full.Y / 100.f, LineHeight / 100.f)));
	}
	for (int32 Index = -RowsY; Index <= RowsY; ++Index)
	{
		Grid->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.f, Index * GridSpacing, 0.5f),
			FVector(Full.X / 100.f, LineWidth / 100.f, LineHeight / 100.f)));
	}

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

	ApplyCamera();
}

void AArena::ApplyCamera()
{
	const float Alpha = FMath::SmoothStep(0.f, 1.f, MenuBlend);

	const float Pitch = FMath::Lerp(CameraPitch, MenuCameraPitch, Alpha);
	const FVector GameLocation = -FRotator(CameraPitch, 0.f, 0.f).Vector() * CameraDistance;
	const FVector MenuLocation = MenuCameraTarget - FRotator(MenuCameraPitch, 0.f, 0.f).Vector() * MenuCameraDistance;

	// Shake: smooth noise (not random jitter), scaled by trauma squared so small knocks stay subtle.
	const float Shake = Trauma * Trauma;
	const FVector ShakeMove = FVector(0.f, FMath::PerlinNoise1D(ShakeTime * 23.f), FMath::PerlinNoise1D(ShakeTime * 23.f + 50.f)) * ShakeOffset * Shake;
	const FRotator ShakeTurn(FMath::PerlinNoise1D(ShakeTime * 19.f + 100.f) * ShakeTilt * Shake, 0.f, FMath::PerlinNoise1D(ShakeTime * 19.f + 150.f) * ShakeTilt * Shake);

	const FRotator Rotation(Pitch, 0.f, 0.f);
	Camera->SetRelativeRotation(Rotation + ShakeTurn);
	Camera->SetRelativeLocation(FMath::Lerp(GameLocation, MenuLocation, Alpha) + Rotation.RotateVector(ShakeMove));
	// The field of view is tuned for 16:9. Unreal keeps the horizontal angle fixed, so a wider screen would crop the
	// arena's top and bottom: widen the angle to keep the 16:9 height in view, and show extra width at the sides.
	const float BaseFieldOfView = FMath::Lerp(CameraFieldOfView, MenuCameraFieldOfView, Alpha);
	const float Widen = FMath::Max(1.f, ViewAspect / (16.f / 9.f));
	const float FieldOfView = FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(BaseFieldOfView) * 0.5f) * Widen));
#if !UE_BUILD_SHIPPING
	// -LWZoom=N narrows the view N times onto the arena centre, to check the hero's model and animation up close.
	static const float DevZoom = [] { float Zoom = 1.f; FParse::Value(FCommandLine::Get(), TEXT("LWZoom="), Zoom); return FMath::Max(Zoom, 1.f); }();
	Camera->SetFieldOfView(FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(FMath::Min(FieldOfView, 170.f)) * 0.5f) / DevZoom)));
#else
	Camera->SetFieldOfView(FMath::Min(FieldOfView, 170.f));
#endif

	// The menu is lit only by the lantern: moonlight and the arena edges fade almost out, and the grid dims so
	// it stays a backdrop behind the buttons.
	if (GridMaterial)
	{
		GridMaterial->SetScalarParameterValue(FName(TEXT("Intensity")), FMath::Lerp(GridGlowIntensity, GridGlowIntensity * 0.3f, Alpha));
	}
	Moonlight->SetIntensity(FMath::Lerp(MoonlightIntensity, MoonlightIntensity * 0.08f, Alpha));
	for (UMaterialInstanceDynamic* Material : EdgeMaterials)
	{
		if (Material)
		{
			Material->SetScalarParameterValue(FName(TEXT("Intensity")), FMath::Lerp(EdgeGlowIntensity, 0.f, Alpha));
		}
	}
}

void AArena::SetLook(const FLinearColor& TopEdge, const FLinearColor& SideEdges, const FLinearColor& GridLines, bool bOpenBottom)
{
	EdgeColor = TopEdge;
	SideEdgeColor = SideEdges;
	GridColor = GridLines;
	EdgeBottom->SetVisibility(!bOpenBottom);
}

void AArena::SetMenuView(bool bMenu, bool bInstant)
{
	MenuBlendTarget = bMenu ? 1.f : 0.f;
	if (bInstant)
	{
		MenuBlend = MenuBlendTarget;
		ApplyCamera();
	}
}

void AArena::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Follow the window's shape (ultrawide, resizing, a resolution change).
	bool bAspectChanged = false;
	if (const UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		FVector2D Size;
		Viewport->GetViewportSize(Size);
		if (Size.X > 0.f && Size.Y > 0.f && !FMath::IsNearlyEqual(Size.X / Size.Y, ViewAspect, 0.001f))
		{
			ViewAspect = Size.X / Size.Y;
			bAspectChanged = true;
		}
	}

	const bool bShaking = Trauma > 0.f;
	if (bShaking)
	{
		ShakeTime += DeltaSeconds;
		Trauma = FMath::Max(0.f, Trauma - TraumaDecay * DeltaSeconds);
	}

	if (MenuBlend != MenuBlendTarget)
	{
		const float Step = DeltaSeconds / FMath::Max(CameraBlendTime, KINDA_SMALL_NUMBER);
		MenuBlend = MenuBlendTarget > MenuBlend ? FMath::Min(MenuBlend + Step, MenuBlendTarget) : FMath::Max(MenuBlend - Step, MenuBlendTarget);
		ApplyCamera();
	}
	else if (bShaking || bAspectChanged)
	{
		ApplyCamera();
	}
}

void AArena::ApplyPostProcess()
{
	LWNeon::SetupPostProcess(PostProcess->Settings, BloomIntensity, ExposureCompensation);
}

FBox2D AArena::GetPlayBounds(float Radius) const
{
	const FVector Centre = GetCentre();
	const FVector2D Inner = FVector2D(
		FMath::Max(0.f, HalfExtent.X - Radius),
		FMath::Max(0.f, HalfExtent.Y - Radius));
	return FBox2D(FVector2D(Centre.X, Centre.Y) - Inner, FVector2D(Centre.X, Centre.Y) + Inner);
}
