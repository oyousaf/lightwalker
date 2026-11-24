#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShieldField.generated.h"

class AShieldPawn;
class ULWParticleComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** What a block of the wall is. Each is one letter in the layouts (ShieldField.cpp). */
enum class EWallBlock : uint8
{
	/** One hit. */
	Shadow,
	/** Two hits. */
	Hard,
	/** Three hits. */
	Stone,
	/** Holds a power-up. */
	Vessel,
	/** Holds a book of knowledge. */
	Book,
	/** Holds a shaytaan, which gets out. */
	Cage,
	/** A dajjal's eye: fires dark bolts. */
	Eye,
	/** The great eye of the last wall: when it breaks, the whole wall does. */
	GreatEye
};

struct FWallBlock
{
	EWallBlock Kind = EWallBlock::Shadow;
	FVector2D Centre = FVector2D::ZeroVector;
	FVector2D Half = FVector2D::ZeroVector;
	int32 HitsLeft = 1;
	float Flash = 0.f;
	/** Seconds until an eye fires. */
	float FireTimer = 3.f;
	/** When it rises into place as the wall appears. */
	float RiseDelay = 0.f;
	TArray<TWeakObjectPtr<USceneComponent>> Parts;
	TArray<FVector> PartScales;
	/** The block itself, which shows as a rim and sides, and the art on its top. */
	TWeakObjectPtr<UMaterialInstanceDynamic> Rim;
	TWeakObjectPtr<UMaterialInstanceDynamic> Face;
	/** An eye's iris, which follows Lightwalker until the eye cracks. */
	TWeakObjectPtr<USceneComponent> Iris;
	bool bCracked = false;
	FLinearColor Color;
};

struct FLightBeam
{
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Direction = FVector2D(1.f, 0.f);
	/** Riding on the shield until it's released. */
	bool bHeld = false;
	float SinceShield = 0.f;
	TWeakObjectPtr<UStaticMeshComponent> Core;
	TWeakObjectPtr<UPointLightComponent> Light;
	TWeakObjectPtr<ULWParticleComponent> Trail;
	float TrailTimer = 0.f;
};

struct FDarkBolt
{
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
	TWeakObjectPtr<UStaticMeshComponent> Core;
	TWeakObjectPtr<UStaticMeshComponent> Halo;
};

/**
 * The Shield's playing field: the wall of darkness, the beams bouncing between it and Lightwalker's shield, and the dark
 * bolts its eyes fire. Everything moves on the arena plane by simple circles and boxes in small fixed steps, so it's
 * exact at any frame rate. The game mode listens to the callbacks and keeps the rules (score, lives, drops).
 */
UCLASS()
class LIGHTWALKER_API AShieldField : public AActor
{
	GENERATED_BODY()

public:
	AShieldField();

	virtual void Tick(float DeltaSeconds) override;

	void Setup(AShieldPawn* InPlayer, const FBox2D& InBounds, float InPlaneZ);

	static int32 GetWallCount();

	/** Raises a wall. */
	void BuildWall(int32 Index);

	/** Removes the wall, beams and bolts. */
	void Clear();
	void ClearBeams();
	void ClearBolts();

	/** Puts a new beam on the shield. */
	void HoldBeam();
	void SplitBeams();

	/** Runs (beams fly, eyes fire) or pauses. */
	void SetRunning(bool bInRunning) { bRunning = bInRunning; }

	int32 GetBlocksLeft() const { return Blocks.Num(); }
	/** 0-1: how much of the wall stands (for the last wall, how much of the great eye). */
	float GetWallLeft() const;
	bool HasGreatEye() const;
	bool HasHeldBeam() const;

	/** Where the next falling beam will cross a line across the arena (for the test bot). */
	bool PredictLanding(float LineX, float& OutY) const;

	// What happened, for the game mode.
	TFunction<void(const FWallBlock&, bool bBroken)> OnBlockHit;
	TFunction<void()> OnDeflect;
	/** The last beam fell. */
	TFunction<void()> OnBeamsLost;
	/** A beam fell and it was the only one: true saves it. */
	TFunction<bool()> OnSaveBeam;
	TFunction<void(const FVector&)> OnBoltBlocked;
	TFunction<void()> OnBoltHitPlayer;
	TFunction<void()> OnWallCleared;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float BeamRadius = 24.f;

	/** The beam's speed at release, what each touch of the shield adds, and its limit. */
	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float BeamSpeed = 1300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float BeamSpeedUp = 1.02f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float BeamTopSpeed = 1750.f;

	/** How far the shield's ends turn the beam. */
	UPROPERTY(EditDefaultsOnly, Category = "Shield", meta = (Units = "deg"))
	float ShieldCurve = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float BoltSpeed = 560.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield", meta = (Units = "s"))
	FVector2D EyeInterval = FVector2D(3.f, 5.f);

	/** Once this few blocks are left, the rest give way rather than making you hunt for them. */
	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	int32 CollapseAt = 7;

private:
	void AddBlock(EWallBlock Kind, float Column, float Row, float Columns = 1.f, float Rows = 1.f);
	void BuildBlockMeshes(FWallBlock& Block);
	void DestroyBlock(int32 Index, bool bBurst);
	FLightBeam& AddBeam(const FVector2D& Position, const FVector2D& Direction);
	void DestroyBeam(int32 Index);
	void DestroyBolt(int32 Index);

	void StepBeam(FLightBeam& Beam, float DeltaSeconds);
	void HitBlocks(FLightBeam& Beam);
	void HitShield(FLightBeam& Beam);
	void HitEnemies(const FLightBeam& Beam);
	void KeepSteep(FLightBeam& Beam) const;
	void UpdateBolts(float DeltaSeconds);
	void UpdateBlocks(float DeltaSeconds);
	void UpdateVisuals(float DeltaSeconds);
	void CheckCleared();

	/** The nearest point of the shield's arc to a point, where along it (-1 to 1), and the direction it faces there. */
	float ShieldDistance(const FVector2D& Point, float& OutAlong, FVector2D& OutNormal) const;

	TArray<FWallBlock> Blocks;
	TArray<FLightBeam> Beams;
	TArray<FDarkBolt> Bolts;
	FBox2D Bounds = FBox2D(ForceInit);
	float PlaneZ = 50.f;
	float WallAge = 0.f;
	float Speed = 1300.f;
	int32 StartingBlocks = 1;
	int32 WallIndex = 0;
	bool bRunning = false;
	bool bCleared = false;

	UPROPERTY(Transient)
	TObjectPtr<AShieldPawn> Player;
};
