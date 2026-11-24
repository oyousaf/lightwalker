#pragma once

#include "CoreMinimal.h"
#include "Core/LWSound.h"
#include "GameFramework/Actor.h"
#include "EnemyBase.generated.h"

class ALightWalkerPawn;
class UMaterialInstanceDynamic;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * A creature of darkness. Subclasses pick their model and decide how they move; the base shows the model, and
 * handles touching the player and dying.
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

	/** Changes how brightly the model glows. */
	void SetModelGlow(float Glow);

	/** Shot by a laser: flashes and loses a hit point, and dies on the last one. */
	virtual void TakeHit();

	/** Tells the game mode, then removes itself. */
	virtual void Kill();
	bool IsDead() const { return bDead; }

protected:
	virtual void BeginPlay() override;

	/** Moves the enemy for this frame. Only called while active. */
	virtual void UpdateMovement(float DeltaSeconds) {}

	/** Animates the body every frame, including while inactive. */
	virtual void UpdateLook(float DeltaSeconds) {}

	ALightWalkerPawn* GetPlayer() const { return Player.Get(); }

	/** Turns the Visual pivot smoothly to face a world direction on the arena plane. */
	void TurnVisualTowards(const FVector& Direction, float DeltaSeconds, float TurnRate);

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USphereComponent> Collision;

	/** Everything visible hangs off this, so the body can turn and bob without moving the collision. */
	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USceneComponent> Visual;

	/**
	 * The imported model (see Tools/import_models.py), scaled so its largest side is ModelSize and centred on the Visual
	 * pivot. A plain glowing ball stands in if it's missing.
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

	/** The burst of light when shot dead (a red ember pop by default). */
	virtual void SpawnDeathEffect();

	/** Played when shot dead (set by each enemy type). */
	ELWSound DeathSound = ELWSound::ShaytaanDeath;

	/**
	 * Leans and squashes the model about the middle of its base, for walk cycles on a static mesh.
	 * Tilt is in the enemy's own frame (roll leans sideways, pitch forwards); Squash scales height, and width
	 * grows to keep the volume.
	 */
	void PoseModel(const FRotator& Tilt, float Squash = 0.f);

	/** Laser hits it takes to destroy. (2014: every enemy died to one shot.) */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker", meta = (ClampMin = 1))
	int32 HitPoints = 1;

	/** Seconds the hit flash and pop take to fade. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float HitFlashTime = 0.18f;

	/** How much a hit swells the body and brightens the model (the boss takes so many that it must be slight). */
	float HitSwell = 0.15f;
	float HitGlow = 4.f;
	float HitFlash = 0.f;
	bool bDead = false;

	/** Radius used for the laser overlap and the player contact check. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float Radius = 50.f;

	FBox2D Bounds = FBox2D(ForceInit);
	float MoveSpeed = 0.f;

	/** Seconds since spawning, offset per enemy so they don't animate in lockstep. */
	float LookTime = 0.f;

private:
	void CheckPlayerContact();
	void ShowModel();

	TWeakObjectPtr<ALightWalkerPawn> Player;

	/** The model's fitted pose, which PoseModel works from. */
	float ModelScale = 1.f;
	FVector ModelOffset = FVector::ZeroVector;
	float ModelHalfHeight = 0.f;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ModelMaterials;

	void UpdateHitFlash(float DeltaSeconds);

	bool bActive = false;
};
