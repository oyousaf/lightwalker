#include "Core/LWNeon.h"

#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

namespace
{
	const TCHAR* NeonPackage = TEXT("/Game/Materials/M_Neon");
	const TCHAR* NeonMaterialPath = TEXT("/Game/Materials/M_Neon.M_Neon");
	const TCHAR* SurfaceMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

	const FName ColorParam(TEXT("Color"));
	const FName IntensityParam(TEXT("Intensity"));

	UMaterialInterface* LoadSurfaceMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, SurfaceMaterialPath);
		}
		return Cached.Get();
	}

	UMaterialInterface* LoadGlowMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			UMaterialInterface* Material = nullptr;
			if (FPackageName::DoesPackageExist(NeonPackage))
			{
				Material = LoadObject<UMaterialInterface>(nullptr, NeonMaterialPath);
			}
			Cached = Material ? Material : LoadSurfaceMaterial();
		}
		return Cached.Get();
	}

	UMaterialInstanceDynamic* Apply(UMeshComponent* Mesh, UMaterialInterface* Base, const FLinearColor& Color, float Intensity)
	{
		if (!Mesh || !Base)
		{
			return nullptr;
		}

		UMaterialInstanceDynamic* Instance = Mesh->CreateDynamicMaterialInstance(0, Base);
		if (Instance)
		{
			Instance->SetVectorParameterValue(ColorParam, Color);
			Instance->SetScalarParameterValue(IntensityParam, Intensity);
		}
		return Instance;
	}
}

UMaterialInstanceDynamic* LWNeon::ApplyGlow(UMeshComponent* Mesh, const FLinearColor& Color, float Intensity)
{
	return Apply(Mesh, LoadGlowMaterial(), Color, Intensity);
}

UMaterialInstanceDynamic* LWNeon::ApplySurface(UMeshComponent* Mesh, const FLinearColor& Color)
{
	return Apply(Mesh, LoadSurfaceMaterial(), Color, 1.f);
}
