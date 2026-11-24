#pragma once

#include "CoreMinimal.h"
#include "Player/LightWalkerPawn.h"
#include "PathPawn.generated.h"

class APathCourse;

/** What happened to Lightwalker on the course this frame, read by the game mode. */
enum class EPathHazard : uint8 { None, Spikes, Fell };

/**
 * Lightwalker on The Path: runs left and right, jumps, and jumps again in the air (the 2014 Green Ball plan). Moves in
 * the YZ plane against APathCourse's boxes; shooting, power-ups and the hero come from the arena's pawn.
 *
 * The jumps are forgiving in the usual ways: a moment to jump after walking off an edge, a press just before landing
 * still counts, and letting go early makes a shorter jump.
 */
UCLASS()
class LIGHTWALKER_API APathPawn : public ALightWalkerPawn
{
	GENERATED_BODY()

public:
	APathPawn();

	virtual void SetJumpHeld(bool bHeld) override;
	virtual bool ProjectCursor(const FVector& Origin, const FVector& Direction, FVector& OutPoint) const override;
	virtual bool IsWithin(const FVector& Point, float Distance) const override;

	void SetCourse(APathCourse* InCourse);

	/** Stands Lightwalker on the ground at (Y, height), still. */
	void PlaceAt(const FVector2D& Spot, float GraceSeconds);

	/** How much light is left, 0-1: scales the lantern. */
	void SetLightLevel(float Level) { LightLevel = Level; }

	/** How far the light reaches, for the course's edges. */
	float GetLightReach() const;

	/** Stops input (before the start, and at the gate). */
	void SetHeld(bool bInHeld) { bHeld = bInHeld; }

	/** Where the camera should follow: steady while jumping, so a hop doesn't bounce the view. */
	FVector GetCameraFocus() const;
	float GetFacing() const { return Facing; }
	float GetFeet() const;
	bool IsGrounded() const { return bGrounded; }

	EPathHazard TakeHazard() { const EPathHazard Taken = Hazard; Hazard = EPathHazard::None; return Taken; }

protected:
	virtual FVector UpdateMovement(const FVector2D& Input, float DeltaSeconds) override;
	virtual FVector AimToWorld(const FVector2D& Screen) const override { return FVector(0.f, Screen.X, Screen.Y); }
	virtual FVector GetCursorDirection() const override;
	virtual FVector GetMuzzle(const FVector& Direction) const override;
	virtual FVector GetFanAxis() const override { return FVector::ForwardVector; }
	virtual FBox2D GetShotBounds() const override;
	virtual float GetLightScale() const override { return FMath::Lerp(0.3f, 1.25f, LightLevel); }
	// The camera is closer than the arena's, so the same light would wash the hero out.
	virtual float GetLightStrength() const override { return 0.4f * GetLightScale(); }
	virtual void FaceDirection(const FVector& Direction) override;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float RunSpeed = 640.f;

	/** How quickly the run reaches full speed and stops, on the ground and in the air (cm/s per second). */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float GroundAcceleration = 6500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float AirAcceleration = 4200.f;

	/** Take-off speeds: the first clears about 250 cm, the second adds about 180. */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float JumpSpeed = 1340.f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float SecondJumpSpeed = 1140.f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float Gravity = 3600.f;

	/** Falls faster than it rises, so jumps feel crisp. */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float FallGravityScale = 1.3f;

	/** Letting go of jump early cuts the rise. */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float CutGravityScale = 2.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float MaxFallSpeed = 1700.f;

	/** Seconds after leaving an edge when a jump still counts as from the ground, and before landing when a press is kept. */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement", meta = (Units = "s"))
	float EdgeForgiveness = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement", meta = (Units = "s"))
	float PressMemory = 0.12f;

	/** Half Lightwalker's width against walls and edges, and against spikes (kinder). */
	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float HalfWidth = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Path|Movement")
	float SpikeHalfWidth = 14.f;

private:
	void Step(float MoveY, float DeltaSeconds);
	void Jump(bool bSecond);
	void Land(int32 BlockIndex);
	void CheckHazards();

#if !UE_BUILD_SHIPPING
	/** -LWAutoPath: plays the course by itself, to check it can be finished. */
	FVector2D AutoPlay(float DeltaSeconds);
	float AutoJumpRelease = 0.f;
	float AutoTrace = 0.f;
	int32 AutoTarget = INDEX_NONE;
#endif

	UPROPERTY(Transient)
	TObjectPtr<APathCourse> Course;

	FVector2D Velocity = FVector2D::ZeroVector;
	bool bGrounded = true;
	bool bJumpHeld = false;
	bool bHeld = false;
	bool bJumpsCut = false;
	int32 JumpsUsed = 0;
	int32 StandingOn = INDEX_NONE;
	float SinceGround = 0.f;
	float SincePress = 1000.f;
	float Facing = 1.f;
	float FacingYaw = 90.f;
	float LightLevel = 1.f;
	float FocusZ = 0.f;
	EPathHazard Hazard = EPathHazard::None;
};
