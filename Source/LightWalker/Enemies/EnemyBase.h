#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyBase.generated.h"

class ALightWalkerPawn;
class USphereComponent;
class UStaticMeshComponent;

/**
 * A creature of darkness. Subclasses decide how it moves; the base handles looks, touching the player and dying.
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

	/** Shot by a laser: tells the game mode, then removes itself. */
	void Kill();
	bool IsDead() const { return bDead; }

protected:
	virtual void BeginPlay() override;

	/** Moves the enemy for this frame. Only called while active. */
	virtual void UpdateMovement(float DeltaSeconds) {}

	ALightWalkerPawn* GetPlayer() const { return Player.Get(); }

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Radius used for the laser overlap and the player contact check. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker")
	float Radius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	FLinearColor Color = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float GlowIntensity = 5.f;

	/** Purely visual spin, degrees per second around Z. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float SpinRate = 0.f;

	FBox2D Bounds = FBox2D(ForceInit);
	float MoveSpeed = 0.f;

private:
	void CheckPlayerContact();

	TWeakObjectPtr<ALightWalkerPawn> Player;
	bool bActive = false;
	bool bDead = false;
};
