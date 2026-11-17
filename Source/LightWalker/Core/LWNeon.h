#pragma once

#include "CoreMinimal.h"

class UMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Shared look for every glowing object in the game.
 *
 * Uses /Game/Materials/M_Neon (unlit, emissive = Color * Intensity) when it exists, and falls back to the
 * engine's BasicShapeMaterial otherwise, so the game still runs before any project content is made.
 */
namespace LWNeon
{
	/** Glowing surface: player, enemies, lasers, arena edges. */
	LIGHTWALKER_API UMaterialInstanceDynamic* ApplyGlow(UMeshComponent* Mesh, const FLinearColor& Color, float Intensity);

	/** Ordinary lit surface (the arena floor), so it picks up light from the glowing objects. */
	LIGHTWALKER_API UMaterialInstanceDynamic* ApplySurface(UMeshComponent* Mesh, const FLinearColor& Color);
}
