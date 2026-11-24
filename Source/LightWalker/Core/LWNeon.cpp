#include "Core/LWNeon.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Scene.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

namespace
{
	constexpr float DesignAspect = 16.f / 9.f;

	UMaterialInterface* LoadSurfaceMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return Cached.Get();
	}

	/** A project material, or the engine's plain one if it hasn't been made. */
	UMaterialInterface* LoadProjectMaterial(TWeakObjectPtr<UMaterialInterface>& Cached, const TCHAR* Name)
	{
		if (!Cached.IsValid())
		{
			const FString Package = FString(TEXT("/Game/Materials/")) + Name;
			UMaterialInterface* Material = FPackageName::DoesPackageExist(Package) ? LoadObject<UMaterialInterface>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
			Cached = Material ? Material : LoadSurfaceMaterial();
		}
		return Cached.Get();
	}

	UMaterialInstanceDynamic* Apply(UMeshComponent* Mesh, UMaterialInterface* Base, const FLinearColor& Color, float Intensity, int32 ElementIndex = 0)
	{
		if (!Mesh || !Base)
		{
			return nullptr;
		}

		UMaterialInstanceDynamic* Instance = Mesh->CreateDynamicMaterialInstance(ElementIndex, Base);
		if (Instance)
		{
			Instance->SetVectorParameterValue(LWNeon::ColorParam, Color);
			Instance->SetScalarParameterValue(LWNeon::IntensityParam, Intensity);
		}
		return Instance;
	}

	template <typename T>
	T* Make(AActor* Owner, const FName& Name, ELWShape Shape, USceneComponent* Parent)
	{
		T* Part = Owner->CreateDefaultSubobject<T>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(LWNeon::Shape(Shape));
		LWNeon::GlowOnly(Part);
		return Part;
	}

	template <typename T>
	T* Add(AActor* Owner, ELWShape Shape, USceneComponent* Parent)
	{
		T* Part = NewObject<T>(Owner);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(LWNeon::Shape(Shape));
		LWNeon::GlowOnly(Part);
		return Part;
	}
}

UMaterialInstanceDynamic* LWNeon::ApplyGlow(UMeshComponent* Mesh, const FLinearColor& Color, float Intensity, int32 ElementIndex)
{
	static TWeakObjectPtr<UMaterialInterface> Neon;
	return Apply(Mesh, LoadProjectMaterial(Neon, TEXT("M_Neon")), Color, Intensity, ElementIndex);
}

UMaterialInstanceDynamic* LWNeon::ApplySurface(UMeshComponent* Mesh, const FLinearColor& Color)
{
	return Apply(Mesh, LoadSurfaceMaterial(), Color, 1.f);
}

UMaterialInstanceDynamic* LWNeon::ApplyRock(UMeshComponent* Mesh, const FLinearColor& Tint)
{
	static TWeakObjectPtr<UMaterialInterface> Rock;
	return Apply(Mesh, LoadProjectMaterial(Rock, TEXT("M_PathRock")), Tint, 1.f);
}

UMaterialInstanceDynamic* LWNeon::ApplyArt(UMeshComponent* Mesh, UTexture2D* Art, float Intensity)
{
	static TWeakObjectPtr<UMaterialInterface> Material;
	UMaterialInstanceDynamic* Instance = Apply(Mesh, LoadProjectMaterial(Material, TEXT("M_Art")), FLinearColor::White, Intensity);
	if (Instance)
	{
		Instance->SetTextureParameterValue(LWNeon::ArtParam, Art);
	}
	return Instance;
}

void LWNeon::SetupPostProcess(FPostProcessSettings& Settings, float BloomIntensity, float ExposureCompensation)
{
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

	// Nothing here to shade (and the screen-space pass hung the GPU on a Radeon RX 7600M XT, 2026-11-24).
	Settings.bOverride_AmbientOcclusionIntensity = true;
	Settings.AmbientOcclusionIntensity = 0.f;

	// Motion blur smears the spinning, overlapping glow shapes into blobs.
	Settings.bOverride_MotionBlurAmount = true;
	Settings.MotionBlurAmount = 0.f;
}

UStaticMesh* LWNeon::Shape(ELWShape Shape)
{
	// Cooked through DirectoriesToAlwaysCook in DefaultGame.ini.
	static const TCHAR* Names[] = { TEXT("Cube"), TEXT("Sphere"), TEXT("Cone"), TEXT("Cylinder"), TEXT("Plane") };
	static TWeakObjectPtr<UStaticMesh> Cache[UE_ARRAY_COUNT(Names)];
	const int32 Index = static_cast<int32>(Shape);
	if (!Cache[Index].IsValid())
	{
		Cache[Index] = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Names[Index], Names[Index]));
	}
	return Cache[Index].Get();
}

void LWNeon::GlowOnly(UPrimitiveComponent* Component)
{
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCastShadow(false);
	Component->bAffectDynamicIndirectLighting = false;
	Component->bAffectDistanceFieldLighting = false;
	Component->bVisibleInRayTracing = false;
}

UStaticMeshComponent* LWNeon::MakeMesh(AActor* Owner, const FName& Name, ELWShape Shape, USceneComponent* Parent)
{
	return Make<UStaticMeshComponent>(Owner, Name, Shape, Parent);
}

UInstancedStaticMeshComponent* LWNeon::MakeInstances(AActor* Owner, const FName& Name, ELWShape Shape, USceneComponent* Parent)
{
	return Make<UInstancedStaticMeshComponent>(Owner, Name, Shape, Parent);
}

UStaticMeshComponent* LWNeon::AddMesh(AActor* Owner, ELWShape Shape, USceneComponent* Parent, const FVector& Location, const FVector& Scale, const FRotator& Rotation)
{
	UStaticMeshComponent* Part = Add<UStaticMeshComponent>(Owner, Shape, Parent);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Scale);
	Part->RegisterComponent();
	return Part;
}

UInstancedStaticMeshComponent* LWNeon::AddInstances(AActor* Owner, ELWShape Shape, USceneComponent* Parent)
{
	UInstancedStaticMeshComponent* Part = Add<UInstancedStaticMeshComponent>(Owner, Shape, Parent);
	Part->RegisterComponent();
	return Part;
}

UStaticMeshComponent* LWNeon::AddCard(AActor* Owner, UTexture2D* Art, USceneComponent* Parent, const FVector& Location, const FVector2D& Size, const FRotator& Rotation, float Intensity)
{
	if (!Art)
	{
		return nullptr;
	}
	UStaticMeshComponent* Card = AddMesh(Owner, ELWShape::Plane, Parent, Location, FVector(Size.X, Size.Y, 100.f) / 100.f, Rotation);
	ApplyArt(Card, Art, Intensity);
	return Card;
}

FRotator LWNeon::CardFacing(const FRotator& Camera)
{
	// The plane's art runs along its +X (across) and +Y (down).
	const FRotationMatrix Axes(Camera);
	return FRotationMatrix::MakeFromXY(Axes.GetScaledAxis(EAxis::Y), -Axes.GetScaledAxis(EAxis::Z)).Rotator();
}

void LWNeon::SetWorldPlaced(USceneComponent* Component)
{
	Component->SetUsingAbsoluteLocation(true);
	Component->SetUsingAbsoluteRotation(true);
	Component->SetUsingAbsoluteScale(true);
}

float LWNeon::GetViewAspect(const UWorld* World)
{
	FVector2D Size = FVector2D::ZeroVector;
	if (const UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr)
	{
		Viewport->GetViewportSize(Size);
	}
	return Size.X > 0.f && Size.Y > 0.f ? Size.X / Size.Y : DesignAspect;
}

float LWNeon::FitFieldOfView(float Degrees, float Aspect)
{
	const float Taller = FMath::Max(1.f, DesignAspect / Aspect);
	return FMath::Min(170.f, FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Degrees) * 0.5f) * Taller)));
}
