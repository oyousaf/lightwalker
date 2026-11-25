#include "Arena/Arena.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LWNeon.h"
#include "Core/LWSeason.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/LWStyle.h"

namespace
{
	constexpr float EdgeThickness = 24.f;
	constexpr float EdgeHeight = 40.f;
	constexpr float EdgeLightWidth = 420.f;
	constexpr float EdgeLightIntensity = 0.11f;
	// The floor runs well past the arena on every side, so even a 32:9 view never sees its edge.
	constexpr float FloorMargin = 30000.f;

	UStaticMeshComponent* MakeMesh(AActor* Owner, const TCHAR* Name, ELWShape Shape, USceneComponent* Parent)
	{
		UStaticMeshComponent* Component = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(Parent);
		Component->SetStaticMesh(LWNeon::Shape(Shape));
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		return Component;
	}
}

AArena::AArena()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Floor = MakeMesh(this, TEXT("Floor"), ELWShape::Plane, RootComponent);
	Grid = LWNeon::MakeInstances(this, TEXT("Grid"), ELWShape::Cube, RootComponent);
	EdgeTop = MakeMesh(this, TEXT("EdgeTop"), ELWShape::Cube, RootComponent);
	EdgeBottom = MakeMesh(this, TEXT("EdgeBottom"), ELWShape::Cube, RootComponent);
	EdgeLeft = MakeMesh(this, TEXT("EdgeLeft"), ELWShape::Cube, RootComponent);
	EdgeRight = MakeMesh(this, TEXT("EdgeRight"), ELWShape::Cube, RootComponent);

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

	RefreshSeason();
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

	// Light spilling from the edges onto the floor: a soft strip under each, on both sides of it.
	EdgeLightMaterials.Reset();
	for (const UStaticMeshComponent* Edge : { EdgeTop.Get(), EdgeBottom.Get(), EdgeLeft.Get(), EdgeRight.Get() })
	{
		const bool bAcross = Edge == EdgeTop || Edge == EdgeBottom;
		const FVector2D Size((bAcross ? HalfExtent.Y : HalfExtent.X) * 2.f + EdgeThickness, EdgeLightWidth);
		const FVector Where = Edge->GetRelativeLocation() * FVector(1.f, 1.f, 0.f) + FVector(0.f, 0.f, 0.3f);
		UStaticMeshComponent* Strip = LWNeon::AddCard(this, LWStyle::Art(TEXT("EdgeLight")), RootComponent, Where, Size, FRotator(0.f, bAcross ? 90.f : 0.f, 0.f), EdgeLightIntensity);
		if (Strip)
		{
			Strip->SetVisibility(Edge->IsVisible());
			Strip->SetTranslucentSortPriority(-1);
		}
		EdgeLightMaterials.Add(Strip ? Cast<UMaterialInstanceDynamic>(Strip->GetMaterial(0)) : nullptr);
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
	const float FieldOfView = LWNeon::FitFieldOfView(FMath::Lerp(CameraFieldOfView, MenuCameraFieldOfView, Alpha), ViewAspect);
#if !UE_BUILD_SHIPPING
	// -LWZoom=N narrows the view N times onto the arena centre, to check the hero's model and animation up close.
	static const float DevZoom = [] { float Zoom = 1.f; FParse::Value(FCommandLine::Get(), TEXT("LWZoom="), Zoom); return FMath::Max(Zoom, 1.f); }();
	Camera->SetFieldOfView(FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(FieldOfView) * 0.5f) / DevZoom)));
#else
	Camera->SetFieldOfView(FieldOfView);
#endif

	// The menu is lit only by the lantern: moonlight and the arena edges fade almost out, and the grid dims so
	// it stays a backdrop behind the buttons.
	if (GridMaterial)
	{
		GridMaterial->SetScalarParameterValue(LWNeon::IntensityParam, FMath::Lerp(GridGlowIntensity, GridGlowIntensity * 0.3f, Alpha));
	}
	const float Gloom = FMath::Max(0.f, Mood);
	const float Dawn = FMath::Max(0.f, -Mood);
	Moonlight->SetIntensity(FMath::Lerp(MoonlightIntensity, MoonlightIntensity * 0.08f, Alpha) * (1.f - 0.75f * Gloom) * (1.f + 2.5f * Dawn));
	Moonlight->SetLightColor(FMath::Lerp(FMath::Lerp(FLinearColor(0.45f, 0.5f, 1.f), FLinearColor(1.f, 0.2f, 0.15f), Gloom), FLinearColor(1.f, 0.72f, 0.4f), Dawn));
	if (GridMaterial)
	{
		GridMaterial->SetVectorParameterValue(LWNeon::ColorParam, FMath::Lerp(FMath::Lerp(GridColor, FLinearColor(0.9f, 0.06f, 0.12f), Gloom), FLinearColor(1.f, 0.6f, 0.2f), Dawn));
	}
	for (int32 Index = 0; Index < EdgeMaterials.Num(); ++Index)
	{
		// Top and bottom first, then the sides (see BeginPlay).
		const FLinearColor Usual = Index < 2 ? EdgeColor : SideEdgeColor;
		const FLinearColor Colour = FMath::Lerp(FMath::Lerp(Usual, FLinearColor(1.f, 0.05f, 0.08f), Gloom), FLinearColor(1.f, 0.65f, 0.2f), Dawn);
		auto Set = [&](UMaterialInstanceDynamic* Material, float Intensity)
		{
			if (Material)
			{
				Material->SetScalarParameterValue(LWNeon::IntensityParam, Intensity * (1.f - Alpha));
				Material->SetVectorParameterValue(LWNeon::ColorParam, Colour);
			}
		};
		Set(EdgeMaterials[Index], EdgeGlowIntensity);
		Set(EdgeLightMaterials.IsValidIndex(Index) ? EdgeLightMaterials[Index].Get() : nullptr, EdgeLightIntensity);
	}
}

void AArena::RefreshSeason()
{
	if (bOwnLook)
	{
		return;
	}
	const AArena* Usual = GetDefault<AArena>();
	EdgeColor = Usual->EdgeColor;
	SideEdgeColor = Usual->SideEdgeColor;
	GridColor = Usual->GridColor;
	LWSeason::GetArenaColors(EdgeColor, SideEdgeColor, GridColor);
	if (HasActorBegunPlay())
	{
		ApplyCamera();  // which sets the colours
	}
}

void AArena::SetLook(const FLinearColor& TopEdge, const FLinearColor& SideEdges, const FLinearColor& GridLines, bool bOpenBottom)
{
	bOwnLook = true;
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
	const float Aspect = LWNeon::GetViewAspect(GetWorld());
	const bool bAspectChanged = !FMath::IsNearlyEqual(Aspect, ViewAspect, 0.001f);
	ViewAspect = Aspect;

	const bool bShaking = Trauma > 0.f;
	if (bShaking)
	{
		ShakeTime += DeltaSeconds;
		Trauma = FMath::Max(0.f, Trauma - TraumaDecay * DeltaSeconds);
	}

	const bool bMoodChanging = Mood != MoodTarget;
	Mood = FMath::FInterpConstantTo(Mood, MoodTarget, DeltaSeconds, 0.6f);

	if (MenuBlend != MenuBlendTarget)
	{
		const float Step = DeltaSeconds / FMath::Max(CameraBlendTime, KINDA_SMALL_NUMBER);
		MenuBlend = MenuBlendTarget > MenuBlend ? FMath::Min(MenuBlend + Step, MenuBlendTarget) : FMath::Max(MenuBlend - Step, MenuBlendTarget);
		ApplyCamera();
	}
	else if (bShaking || bAspectChanged || bMoodChanging)
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
