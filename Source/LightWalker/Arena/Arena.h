#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arena.generated.h"

class UCameraComponent;
class UDirectionalLightComponent;
class UPostProcessComponent;
class UInstancedStaticMeshComponent;
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

	virtual void Tick(float DeltaSeconds) override;

	/** Moves the camera between the low menu close-up of Lightwalker and the top-down game view. */
	void SetMenuView(bool bMenu, bool bInstant = false);

	/** Another mode's colours for this arena, and no bottom edge (The Shield). Call before play begins. */
	void SetLook(const FLinearColor& TopEdge, const FLinearColor& SideEdges, const FLinearColor& GridLines, bool bOpenBottom);

	/** The arena's mood, eased into: 1 is the boss's gloom (dark and red), -1 the dawn after it, 0 as normal. */
	void SetMood(float InMood) { MoodTarget = InMood; }

	/** Shakes the camera: trauma (0-1) adds up, decays over time, and the shake grows with its square. */
	void AddTrauma(float Amount) { Trauma = FMath::Min(1.f, Trauma + Amount); }

protected:
	virtual void BeginPlay() override;

	/** Menu close-up: the point the camera looks at (relative to the centre; -Y puts Lightwalker right of centre). */
	UPROPERTY(EditAnywhere, Category = "Arena|Menu Camera")
	FVector MenuCameraTarget = FVector(180.f, -170.f, 60.f);

	UPROPERTY(EditAnywhere, Category = "Arena|Menu Camera", meta = (ClampMin = -90, ClampMax = 0))
	float MenuCameraPitch = -20.f;

	UPROPERTY(EditAnywhere, Category = "Arena|Menu Camera")
	float MenuCameraDistance = 820.f;

	UPROPERTY(EditAnywhere, Category = "Arena|Menu Camera")
	float MenuCameraFieldOfView = 45.f;

	/** Seconds for the sweep between the menu and game views. */
	UPROPERTY(EditAnywhere, Category = "Arena|Menu Camera", meta = (Units = "s"))
	float CameraBlendTime = 1.8f;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UStaticMeshComponent> Floor;

	/** Faint neon grid lines across the floor, like the sunset art's gridscape. */
	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UInstancedStaticMeshComponent> Grid;

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
	FLinearColor FloorColor = FLinearColor(0.016f, 0.008f, 0.032f);

	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	FLinearColor EdgeColor = FLinearColor(1.f, 0.1f, 0.45f);

	/** The left and right edges, so the arena is framed in both neon colours. */
	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	FLinearColor SideEdgeColor = FLinearColor(0.2f, 0.85f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	float EdgeGlowIntensity = 2.2f;

	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	FLinearColor GridColor = FLinearColor(0.35f, 0.25f, 1.f);

	/** Kept low: the grid gives the floor depth without competing with the enemies. */
	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	float GridGlowIntensity = 0.36f;

	/** Distance between grid lines. */
	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	float GridSpacing = 206.f;

	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	float MoonlightIntensity = 1.5f;

	/** Fixed exposure, so the dark arena doesn't brighten itself up. 0 means emissive 1 shows as roughly full brightness. */
	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	float ExposureCompensation = 0.f;

	UPROPERTY(EditAnywhere, Category = "Arena|Look")
	float BloomIntensity = 1.2f;

private:
	void Layout();
	void ApplyPostProcess();
	void ApplyCamera();

	UPROPERTY(Transient)
	TArray<TObjectPtr<class UMaterialInstanceDynamic>> EdgeMaterials;

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> GridMaterial;

	/** Camera shake: current trauma, how fast it fades per second, the largest offset (cm) and tilt (degrees). */
	float Trauma = 0.f;
	float TraumaDecay = 1.6f;
	float ShakeOffset = 28.f;
	float ShakeTilt = 1.2f;
	float ShakeTime = 0.f;

	/** The game window's width over height; the camera widens past 16:9 so ultrawide screens keep the whole arena. */
	float ViewAspect = 16.f / 9.f;

	float Mood = 0.f;
	float MoodTarget = 0.f;

	/** 0 = game view, 1 = menu view. */
	float MenuBlend = 0.f;
	float MenuBlendTarget = 0.f;
};
