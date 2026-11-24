#pragma once

#include "CoreMinimal.h"

class UMeshComponent;
class UMaterialInstanceDynamic;
struct FPostProcessSettings;

/**
 * Shared look for every glowing object in the game.
 *
 * Uses /Game/Materials/M_Neon (unlit, emissive = Color * Intensity) when it exists, and falls back to the
 * engine's BasicShapeMaterial otherwise, so the game still runs before any project content is made.
 */
namespace LWNeon
{
	/** Glowing surface: player, enemies, lasers, arena edges. */
	LIGHTWALKER_API UMaterialInstanceDynamic* ApplyGlow(UMeshComponent* Mesh, const FLinearColor& Color, float Intensity, int32 ElementIndex = 0);

	/** Ordinary lit surface (the arena floor), so it picks up light from the glowing objects. */
	LIGHTWALKER_API UMaterialInstanceDynamic* ApplySurface(UMeshComponent* Mesh, const FLinearColor& Color);

	/** The post-processing every mode shares: fixed exposure (so the dark doesn't brighten itself up), bloom, no motion blur. */
	LIGHTWALKER_API void SetupPostProcess(FPostProcessSettings& Settings, float BloomIntensity = 1.2f, float ExposureCompensation = 0.f);
}
