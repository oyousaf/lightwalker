#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PathCourse.generated.h"

class APathOrb;
class UCameraComponent;
class UDirectionalLightComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPostProcessComponent;
class UStaticMeshComponent;

/** What a block of the course does. */
enum class EPathBlock : uint8
{
	/** Solid ground, down into the abyss: blocks from the sides too. */
	Ground,
	/** Thin floating ledge: landed on from above, jumped through from below. */
	Ledge,
	/** A ledge that shakes and falls a moment after it's stood on, then comes back. */
	Crumbling,
	/** A ledge that glides to and fro, carrying whoever stands on it. */
	Moving
};

struct FPathBlock
{
	EPathBlock Kind = EPathBlock::Ground;
	float Left = 0.f;
	float Right = 0.f;
	float Top = 0.f;

	/** Moving: how far it travels from its start (Y, Z), and seconds for there and back. */
	FVector2D Travel = FVector2D::ZeroVector;
	float Period = 4.f;

	/** Current offset from its start, how far it moved this frame, and how visible its edge is. */
	FVector2D Offset = FVector2D::ZeroVector;
	FVector2D FrameMove = FVector2D::ZeroVector;

	/** Crumbling: seconds since stood on (negative = untouched). */
	float CrumbleTime = -1.f;

	int32 Part = INDEX_NONE;

	float GetLeft() const { return Left + Offset.X; }
	float GetRight() const { return Right + Offset.X; }
	float GetTop() const { return Top + Offset.Y; }
	bool IsSolid() const { return Kind != EPathBlock::Crumbling || CrumbleTime < 0.55f || CrumbleTime > 3.2f; }
};

/** A row of spikes on the ground. */
struct FPathSpikes
{
	float Left = 0.f;
	float Right = 0.f;
	float Base = 0.f;
};

/** Where something stands on the course; the game mode spawns the enemies and the book. */
enum class EPathSpawn : uint8 { Shaytaan, Dajjal, Book };
struct FPathSpawn
{
	EPathSpawn Kind;
	float Y;
	float Z;
	/** Shaytaan: the stretch of ground they patrol. */
	float PatrolLeft = 0.f;
	float PatrolRight = 0.f;
};

/**
 * The Path's course and everything around it: the ground, ledges and spikes (laid out in BuildCourse), the checkpoint
 * lanterns, the gate of light at the end, the neon abyss and sunset behind, and the side-on camera.
 *
 * Side-on: screen right is world +Y, up is +Z, and the camera looks along +X. Collision is simple boxes tested by
 * APathPawn, so it's exact and the same at any frame rate.
 */
UCLASS()
class LIGHTWALKER_API APathCourse : public AActor
{
	GENERATED_BODY()

public:
	APathCourse();

	virtual void Tick(float DeltaSeconds) override;

	const TArray<FPathBlock>& GetBlocks() const { return Blocks; }
	const TArray<FPathSpikes>& GetSpikes() const { return Spikes; }
	const TArray<FPathSpawn>& GetSpawns() const { return Spawns; }
	const TArray<FVector2D>& GetOrbSpots() const { return OrbSpots; }

	/** Checkpoints in order (Y, ground height); the first is the start. */
	const TArray<FVector2D>& GetCheckpoints() const { return Checkpoints; }
	float GetGateY() const { return GateY; }

	/** The ground the course has at its lowest near Y; falling well below it is falling into the dark. */
	float GetLowestNear(float Y) const;

	/** Lights a checkpoint's lantern. */
	void LightCheckpoint(int32 Index);

	/** A crumbling ledge was stood on. */
	void StandOn(int32 BlockIndex);

	/** Puts every crumbling ledge back. */
	void ResetCrumbling();

	bool IsSolidAt(float Y, float Z) const;

	/** What the camera follows, and how far Lightwalker's light reaches (the edges inside it glow). */
	void Follow(const FVector& Location, float Facing, float LightReach, bool bInstant = false);

	/** The whole world dims as the lantern (0-1) runs down. */
	void SetLight(float Level) { LightLevel = Level; }

	void AddTrauma(float Amount) { Trauma = FMath::Min(1.f, Trauma + Amount); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Path")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Path")
	TObjectPtr<UDirectionalLightComponent> Moonlight;

	UPROPERTY(VisibleAnywhere, Category = "Path")
	TObjectPtr<UPostProcessComponent> PostProcess;

	/** Follows the camera: the sky, stars and sun. */
	UPROPERTY(VisibleAnywhere, Category = "Path")
	TObjectPtr<USceneComponent> Sky;

	/** Rises behind the ridges as the gate gets nearer. */
	UPROPERTY(VisibleAnywhere, Category = "Path")
	TObjectPtr<USceneComponent> Sun;

	UPROPERTY(EditAnywhere, Category = "Path|Camera")
	float CameraDistance = 1650.f;

	UPROPERTY(EditAnywhere, Category = "Path|Camera")
	float CameraPitch = -6.f;

	UPROPERTY(EditAnywhere, Category = "Path|Camera")
	float CameraFieldOfView = 58.f;

	/** How far ahead of Lightwalker the camera looks, and how far above their feet. */
	UPROPERTY(EditAnywhere, Category = "Path|Camera")
	float LookAhead = 260.f;

	UPROPERTY(EditAnywhere, Category = "Path|Camera")
	float LookUp = 190.f;

	/** Tints the rock. */
	UPROPERTY(EditAnywhere, Category = "Path|Look")
	FLinearColor GroundColor = FLinearColor(1.f, 1.f, 1.f);

	/** Edge glow in the light, and the faint trace left in the dark. */
	UPROPERTY(EditAnywhere, Category = "Path|Look")
	float EdgeGlow = 1.8f;

	UPROPERTY(EditAnywhere, Category = "Path|Look")
	float EdgeGlowInDark = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Path|Look")
	float MoonlightIntensity = 0.25f;

private:
	/** The layout, in order from the start to the gate. */
	void BuildCourse();

	// Course-building steps. Each works from the cursor: where the course has reached, and the height of the ground there.
	void Ground(float Length);
	void Gap(float Length) { CursorY += Length; }
	void Rise(float Height) { CursorZ += Height; }
	void AddBlock(EPathBlock Kind, float FromCursor, float Width, float Height, const FVector2D& Travel = FVector2D::ZeroVector, float Period = 4.f);
	/** Spikes on the ground, measured back from the cursor. */
	void SpikesBack(float Back, float Width);
	/** Orbs in an arc: measured from the cursor, Count of them over Span, Height above the ground, Arc higher in the middle. */
	void Orbs(float FromCursor, int32 Count, float Span, float Height, float Arc = 0.f);
	void Enemy(EPathSpawn Kind, float Back, float Height, float PatrolBack = 0.f, float PatrolFront = 0.f);
	void Checkpoint(float Back);

	void BuildMeshes();
	void BuildBackdrop();
	void UpdateBlocks(float DeltaSeconds);
	void UpdateCamera(float DeltaSeconds, bool bInstant);

	TArray<FPathBlock> Blocks;
	TArray<FPathSpikes> Spikes;
	TArray<FPathSpawn> Spawns;
	TArray<FVector2D> OrbSpots;
	TArray<FVector2D> Checkpoints;
	float GateY = 0.f;
	float CursorY = 0.f;
	float CursorZ = 0.f;
	float CourseTime = 0.f;

	/** A block's meshes and the glow of its edge. */
	struct FPart
	{
		TWeakObjectPtr<UStaticMeshComponent> Body;
		TWeakObjectPtr<UStaticMeshComponent> Edge;
		TWeakObjectPtr<UMaterialInstanceDynamic> EdgeMaterial;
		FVector BodyStart;
		FVector EdgeStart;
	};
	TArray<FPart> Parts;

	struct FSpikeRow
	{
		TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Materials;
		float Centre = 0.f;
	};
	TArray<FSpikeRow> SpikeRows;

	struct FLantern
	{
		/** The post with its lantern out, and the same with it lit, laid over it. */
		TWeakObjectPtr<UMaterialInstanceDynamic> Post;
		TWeakObjectPtr<UMaterialInstanceDynamic> Lit;
		TWeakObjectPtr<class UPointLightComponent> Light;
		float Y = 0.f;
		bool bLit = false;
		float Blend = 0.f;
	};
	TArray<FLantern> Lanterns;

	TWeakObjectPtr<UMaterialInstanceDynamic> GateMaterial;

	FVector Target = FVector::ZeroVector;
	FVector CameraAt = FVector::ZeroVector;
	float TargetFacing = 1.f;
	float FacingBlend = 1.f;
	float Reach = 700.f;
	float LightLevel = 1.f;
	float ShownLight = 1.f;
	float Trauma = 0.f;
	float ShakeTime = 0.f;
};
