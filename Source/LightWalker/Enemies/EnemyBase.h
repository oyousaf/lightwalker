#pragma once

#include "CoreMinimal.h"
#include "Core/LWSound.h"
#include "GameFramework/Actor.h"
#include "EnemyBase.generated.h"

class ALightWalkerPawn;
class UMaterialInstanceDynamic;
class UMeshComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/** One glowing piece of an enemy's body, coloured when the enemy spawns. */
USTRUCT()
struct FEnemyGlowPart
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UMeshComponent> Mesh;

	FLinearColor Color = FLinearColor::White;
	float Intensity = 1.f;

	/** Material slot, for meshes with several sections. */
	int32 Element = 0;

	/** Unlit black instead of glowing (pupils and other dark details). */
	bool bDark = false;
};

/**
 * A creature of darkness. Subclasses build their look from parts on the Visual pivot and decide how they move;
 * the base handles colouring, touching the player and dying.
 */
UCLASS(Abstract)
class LIGHTWALKER_API AEnemyBase : public AActor
{
	GENERATED_BODY()

public:
	AEnemyBase();

	virtual void Tick(float DeltaSeconds) override;

	/** Called by the game mode straight after spawning. */
	virtual void InitEnemy(ALightWalkerPawn* InPlayer, const FBox2D& InBounds, float InMoveSpeed);

	/** Inactive enemies are visible but don't move or hurt (wave intro, game over). */
	void SetActive(bool bInActive) { bActive = bInActive; }

	void SetMoveSpeed(float InMoveSpeed) { MoveSpeed = InMoveSpeed; }
	float GetMoveSpeed() const { return MoveSpeed; }

	/** Changes how brightly the imported model glows (lurkers in the menu are dimmed). */
	void SetModelGlow(float Glow);

	/** Shot by a laser: flashes and loses a hit point, and dies on the last one. */
	void TakeHit();

	/** Tells the game mode, then removes itself. */
	void Kill();
	bool IsDead() const { return bDead; }

protected:
	virtual void BeginPlay() override;

	/** Moves the enemy for this frame. Only called while active. */
	virtual void UpdateMovement(float DeltaSeconds) {}

	/** Animates the body every frame, including while inactive. */
	virtual void UpdateLook(float DeltaSeconds) {}

	ALightWalkerPawn* GetPlayer() const { return Player.Get(); }

	/** Adds an engine basic shape (Sphere, Cube, Cone, Cylinder) to the Visual pivot. Constructor only. */
	UStaticMeshComponent* AddShape(FName Name, UStaticMesh* Mesh, USceneComponent* Parent,
		const FVector& Location, const FRotator& Rotation, const FVector& Scale);

	/** Registers a mesh to be coloured in BeginPlay. */
	void AddGlowPart(UMeshComponent* Mesh, const FLinearColor& InColor, float Intensity, bool bDark = false, int32 Element = 0);

	/** Adds a thin glowing rod from Start to End (relative to Parent). Constructor only. */
	UStaticMeshComponent* AddRod(FName Name, UStaticMesh* CylinderMesh, USceneComponent* Parent, const FVector& Start, const FVector& End, float Thickness);

	/** Marks a placeholder shape that is hidden once a real model (ModelPath) is in use. */
	void AddPlaceholder(USceneComponent* Component) { Placeholders.Add(Component); }

	/** Turns the Visual pivot smoothly to face a world direction on the arena plane. */
	void TurnVisualTowards(const FVector& Direction, float DeltaSeconds, float TurnRate);

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USphereComponent> Collision;

	/** Everything visible hangs off this, so the body can turn and bob without moving the collision. */
	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USceneComponent> Visual;

	UPROPERTY()
	TArray<FEnemyGlowPart> GlowParts;

	/**
	 * Optional imported model (see Tools/import_models.py). When the asset exists it replaces the placeholder shapes,
	 * scaled so its largest side is ModelSize and centred on the Visual pivot; trails and other effects stay.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Model")
	FSoftObjectPath ModelPath;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Model")
	float ModelSize = 100.f;

	/** Turns the model so its front faces +X (the way the enemy faces). glTF models face +Y once imported, hence -90. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Model")
	float ModelYaw = -90.f;

	/** How brightly the model shows its own texture colours, so it reads in the dark arena (0 = lit by the scene only). */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Model")
	float ModelGlow = 1.5f;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker|Model")
	TObjectPtr<UStaticMeshComponent> Model;

	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Placeholders;

	/** Played when shot dead (set by each enemy type). */
	ELWSound DeathSound = ELWSound::ShaytaanDeath;

	/** True once an imported model replaced the placeholders. */
	bool bUsingModel = false;

	/**
	 * Leans and squashes the imported model about the middle of its base, for walk cycles on a static mesh.
	 * Tilt is in the enemy's own frame (roll leans sideways, pitch forwards); Squash scales height, and width
	 * grows to keep the volume. Does nothing while the placeholders are showing.
	 */
	void PoseModel(const FRotator& Tilt, float Squash = 0.f);

	/** Laser hits it takes to destroy. (2014: every enemy died to one shot.) */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker", meta = (ClampMin = 1))
	int32 HitPoints = 1;

	/** Seconds the hit flash and pop take to fade. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float HitFlashTime = 0.18f;

	/** Radius used for the laser overlap and the player contact check. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float Radius = 50.f;

	FBox2D Bounds = FBox2D(ForceInit);
	float MoveSpeed = 0.f;

	/** Seconds since spawning, offset per enemy so they don't animate in lockstep. */
	float LookTime = 0.f;

private:
	void CheckPlayerContact();
	void TryUseModel();

	TWeakObjectPtr<ALightWalkerPawn> Player;

	/** The model's fitted pose, which PoseModel works from. */
	float ModelScale = 1.f;
	FVector ModelOffset = FVector::ZeroVector;
	float ModelHalfHeight = 0.f;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ModelMaterials;

	void UpdateHitFlash(float DeltaSeconds);
	float HitFlash = 0.f;

	bool bActive = false;
	bool bDead = false;
};
