#pragma once

#include "CoreMinimal.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;
struct FPostProcessSettings;

/** The engine's basic shapes: 100 units across and centred on their pivot (the cone and cylinder run along +Z). */
enum class ELWShape : uint8 { Cube, Sphere, Cone, Cylinder, Plane };

/**
 * Shared look and building blocks for every glowing object in the game.
 *
 * Glow uses /Game/Materials/M_Neon (unlit, emissive = Color * Intensity), and art on cards and blocks uses
 * /Game/Materials/M_Art (unlit, translucent, the art * Intensity). Both fall back to the engine's BasicShapeMaterial.
 */
namespace LWNeon
{
	/** Parameters of both materials. */
	inline const FName ColorParam(TEXT("Color"));
	inline const FName IntensityParam(TEXT("Intensity"));
	/** M_Art's: the picture, and how solid it is. */
	inline const FName ArtParam(TEXT("Art"));
	inline const FName OpacityParam(TEXT("Opacity"));
	/** M_ModelGlow's: how brightly an imported model shows its own colours. */
	inline const FName GlowParam(TEXT("Glow"));

	/** Glowing surface: player, enemies, lasers, arena edges. */
	LIGHTWALKER_API UMaterialInstanceDynamic* ApplyGlow(UMeshComponent* Mesh, const FLinearColor& Color, float Intensity, int32 ElementIndex = 0);

	/** Ordinary lit surface (the arena floor), so it picks up light from the glowing objects. */
	LIGHTWALKER_API UMaterialInstanceDynamic* ApplySurface(UMeshComponent* Mesh, const FLinearColor& Color);

	/** The Path's rock: lit, textured by where it is in the world. */
	LIGHTWALKER_API UMaterialInstanceDynamic* ApplyRock(UMeshComponent* Mesh, const FLinearColor& Tint);

	/** Artwork on a mesh, keeping its transparency. */
	LIGHTWALKER_API UMaterialInstanceDynamic* ApplyArt(UMeshComponent* Mesh, UTexture2D* Art, float Intensity = 1.f);

	/** The post-processing every mode shares: fixed exposure (so the dark doesn't brighten itself up), bloom, no motion blur. */
	LIGHTWALKER_API void SetupPostProcess(FPostProcessSettings& Settings, float BloomIntensity = 1.2f, float ExposureCompensation = 0.f);

	LIGHTWALKER_API UStaticMesh* Shape(ELWShape Shape);

	/** Drawn and nothing else: no collision or shadow, and kept out of indirect lighting (where moving glows leave ghosts). */
	LIGHTWALKER_API void GlowOnly(UPrimitiveComponent* Component);

	// Glow-only parts. Make is for constructors, Add for during play (locations are relative to the parent).
	LIGHTWALKER_API UStaticMeshComponent* MakeMesh(AActor* Owner, const FName& Name, ELWShape Shape, USceneComponent* Parent);
	LIGHTWALKER_API UInstancedStaticMeshComponent* MakeInstances(AActor* Owner, const FName& Name, ELWShape Shape, USceneComponent* Parent);
	LIGHTWALKER_API UStaticMeshComponent* AddMesh(AActor* Owner, ELWShape Shape, USceneComponent* Parent, const FVector& Location, const FVector& Scale,
		const FRotator& Rotation = FRotator::ZeroRotator);
	LIGHTWALKER_API UInstancedStaticMeshComponent* AddInstances(AActor* Owner, ELWShape Shape, USceneComponent* Parent);

	/** Artwork standing in the world: Size is its width and height, and Rotation comes from CardFacing. Null without the art. */
	LIGHTWALKER_API UStaticMeshComponent* AddCard(AActor* Owner, UTexture2D* Art, USceneComponent* Parent, const FVector& Location, const FVector2D& Size,
		const FRotator& Rotation, float Intensity = 1.f);

	/** The rotation that turns a card to face a camera. */
	LIGHTWALKER_API FRotator CardFacing(const FRotator& Camera);

	/** Placed in the world by its own code rather than carried by its parent. */
	LIGHTWALKER_API void SetWorldPlaced(USceneComponent* Component);

	/** The game view's width over its height (16:9 when there's no view yet). */
	LIGHTWALKER_API float GetViewAspect(const UWorld* World);

	/**
	 * Cameras are tuned for 16:9, and the engine keeps that height on any screen (AspectRatio_MaintainYFOV in DefaultEngine.ini),
	 * so wider ones simply see further to the sides. On a narrower one, this is the angle that still shows the full width.
	 */
	LIGHTWALKER_API float FitFieldOfView(float Degrees, float Aspect);
}
