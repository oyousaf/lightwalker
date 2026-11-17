#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LightWalkerPawn.generated.h"

class ALaser;
class UPointLightComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Lightwalker, the player. Moves on the arena plane and fires lasers.
 *
 * Fire direction, in priority order: the right stick, the mouse cursor while the mouse button is held,
 * then the way Lightwalker is facing (last movement) while a fire button is held.
 *
 * Input arrives as screen-space 2D vectors (x = right, y = up) from ALightWalkerPlayerController.
 * With the top-down camera, screen up is world +X and screen right is world +Y.
 */
UCLASS()
class LIGHTWALKER_API ALightWalkerPawn : public APawn
{
	GENERATED_BODY()

public:
	ALightWalkerPawn();

	virtual void Tick(float DeltaSeconds) override;

	void SetMoveInput(const FVector2D& ScreenInput) { MoveInput = ScreenInput; }
	void SetAimInput(const FVector2D& ScreenInput) { AimInput = ScreenInput; }
	void SetFireHeld(bool bHeld) { bFireHeld = bHeld; }

	/** Mouse aiming: fire towards a world point while active. */
	void SetCursorAim(bool bActive, const FVector& WorldTarget = FVector::ZeroVector) { bCursorAim = bActive; CursorTarget = WorldTarget; }

	/** World-space XY area the pawn's centre must stay inside. */
	void SetPlayBounds(const FBox2D& Bounds) { PlayBounds = Bounds; }

	/** Hides the pawn and stops it acting (killed, or between waves). */
	void SetDowned(bool bInDowned);

	/** Brings the pawn back at Location, untouchable for GraceSeconds. */
	void Revive(const FVector& Location, float GraceSeconds);

	bool IsDowned() const { return bDowned; }
	bool IsVulnerable() const { return !bDowned && GraceRemaining <= 0.f; }
	float GetContactRadius() const { return ContactRadius; }

	/** Converts a screen-space stick/key vector to a world direction on the arena plane. */
	static FVector ScreenToWorld(const FVector2D& Screen) { return FVector(Screen.Y, Screen.X, 0.f); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Rotates to face the last aim (or move) direction. */
	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<USceneComponent> HeadingPivot;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<UStaticMeshComponent> Pointer;

	UPROPERTY(VisibleAnywhere, Category = "LightWalker")
	TObjectPtr<UPointLightComponent> Glow;

	/** 2014: 6 px per frame at 24 fps, scaled 5 units per pixel. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Movement")
	float MoveSpeed = 720.f;

	/** Used for enemy contact checks. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Movement")
	float ContactRadius = 45.f;

	/** 2014: SHOTTIMER of 7 frames at 24 fps. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Laser")
	float FireInterval = 0.29f;

	/** Aim input below this length doesn't fire. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Laser")
	float AimThreshold = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Laser")
	TSubclassOf<ALaser> LaserClass;

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	FLinearColor Color = FLinearColor(0.1f, 0.85f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float GlowIntensity = 6.f;

	/** How fast the pawn blinks while untouchable after a respawn. */
	UPROPERTY(EditDefaultsOnly, Category = "LightWalker|Look")
	float GraceBlinkRate = 10.f;

private:
	void Fire(const FVector& Direction);
	void FaceDirection(const FVector& Direction);
	void SetBodyVisible(bool bVisible);

	FVector2D MoveInput = FVector2D::ZeroVector;
	FVector2D AimInput = FVector2D::ZeroVector;
	bool bFireHeld = false;
	bool bCursorAim = false;
	FVector CursorTarget = FVector::ZeroVector;

	/** Last movement direction; starts facing screen up. */
	FVector Heading = FVector::ForwardVector;
	FBox2D PlayBounds = FBox2D(ForceInit);
	float FireCooldown = 0.f;
	float GraceRemaining = 0.f;
	bool bDowned = false;
};
