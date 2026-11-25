#pragma once

#include "CoreMinimal.h"
#include "Core/LWSeason.h"
#include "GameFramework/Actor.h"
#include "LWWeather.generated.h"

class UInstancedStaticMeshComponent;

/** The season's weather in the world: snow, rain, petals, leaves or embers drifting through a box of air. */
UCLASS()
class LIGHTWALKER_API ALWWeather : public AActor
{
	GENERATED_BODY()

public:
	ALWWeather();

	/** The air it fills, and whether the box travels with the camera (The Path). */
	void Setup(const FBox& InAir, bool bInFollowCamera);

	/** Puts up the current season's, if it isn't already. */
	void Refresh();

	virtual void Tick(float DeltaSeconds) override;

private:
	struct FThing
	{
		FVector Position;
		float Fall;
		float Size;
		float Phase;
	};
	struct FLayer
	{
		FLWWeatherLayer Look;
		TArray<FThing> Things;
		TArray<FTransform> Transforms;
	};
	TArray<FLayer> Layers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Meshes;

	FBox Air = FBox(ForceInit);
	bool bFollowCamera = false;
	ELWSeason Shown = ELWSeason::None;
	float Time = 0.f;
};
