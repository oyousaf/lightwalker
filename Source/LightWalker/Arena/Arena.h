#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arena.generated.h"

class UCameraComponent;
class UDirectionalLightComponent;
class UPostProcessComponent;
class UStaticMeshComponent;

/**
 * The playing field: dark floor, glowing edges, the fixed top-down camera, lighting and post-processing.
 * The game mode spawns one if the level doesn't already contain one, so any empty level can host the arena.
 *
 * Screen up is world +X and screen right is world +Y.
 */
UCLASS()
class LIGHTWALKER_API AArena : public AActor
{
	GENERATED_BODY()

public:
	AArena();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** World-space XY area an object of the given radius can occupy without crossing the edges. */
	FBox2D GetPlayBounds(float Radius) const;

	FVector GetCentre() const { return GetActorLocation(); }
	FVector2D GetHalfExtent() const { return HalfExtent; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UStaticMeshComponent> Floor;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UStaticMeshComponent> EdgeTop;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UStaticMeshComponent> EdgeBottom;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UStaticMeshComponent> EdgeLeft;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UStaticMeshComponent> EdgeRight;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UDirectionalLightComponent> Moonlight;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UPostProcessComponent> PostProcess;

	/** Half the playable size: X is screen height, Y is screen width. 2014: 604 x 413 px, scaled 5 units per pixel. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	FVector2D HalfExtent = FVector2D(1030.f, 1510.f);

	/** Camera tilt below the horizon. -90 looks straight down. */
	UPROPERTY(EditAnywhere, Category = "Arena|Camera", meta = (ClampMin = -90, ClampMax = -20))
	float CameraPitch = -58.f;

	UPROPERTY(EditAnywhere, Category = "Arena|Camera")
	float CameraDistance = 3700.f;

	UPROPERTY(EditAnywhere, Category = "Arena|Camera")
	float CameraFieldOfView = 55.f;

	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	FLinearColor FloorColor = FLinearColor(0.012f, 0.014f, 0.03f);

	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	FLinearColor EdgeColor = FLinearColor(0.35f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	float EdgeGlowIntensity = 4.f;

	/** Fixed exposure, so the dark arena doesn't brighten itself up. 0 means emissive 1 shows as roughly full brightness. */
	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	float ExposureCompensation = 0.f;

	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	float BloomIntensity = 1.2f;

private:
	void Layout();
	void ApplyPostProcess();
};
