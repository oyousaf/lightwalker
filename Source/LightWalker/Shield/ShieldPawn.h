#pragma once

#include "CoreMinimal.h"
#include "Player/LightWalkerPawn.h"
#include "ShieldPawn.generated.h"

class AShieldField;
class UMaterialInstanceDynamic;

/**
 * Lightwalker on The Shield: slides left and right along the bottom of the arena behind a curved shield of light, and
 * tilts it (right stick, or towards the mouse while it's held). Firing releases the beam.
 */
UCLASS()
class LIGHTWALKER_API AShieldPawn : public ALightWalkerPawn
{
	GENERATED_BODY()

public:
	AShieldPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void CollectPickup(EPickupType Type) override;

	void SetField(AShieldField* InField) { Field = InField; }
	void SetHeld(bool bInHeld) { bHeld = bInHeld; }

	/** Back to the middle of the line. */
	void PlaceAt(const FVector& Location, float GraceSeconds);

	// The shield: a shallow arc, facing up the arena when level.
	FVector2D GetShieldCentre() const;
	FVector2D GetShieldFacing() const;
	float GetShieldHalfWidth() const { return ShownHalfWidth; }
	/** How far back the arc bends at its ends. */
	float GetShieldBend() const { return ShieldBend; }

	/** A blow on the shield: it narrows for a while. */
	void StrainShield();
	float GetStrainLeft() const { return StrainLeft; }

	/** Brightens the shield for a moment (the beam struck it). */
	void FlashShield() { ShieldFlash = 1.f; }

	bool WantsRelease() const { return !bHeld && (bFireHeld || bCursorAim || bAutoRelease); }

	/** A pickup for the field rather than the hero (the spread shot splits the beam). */
	TFunction<void(EPickupType)> OnPickup;

protected:
	virtual void BeginPlay() override;
	virtual FVector UpdateMovement(const FVector2D& Input, float DeltaSeconds) override;
	virtual void FaceDirection(const FVector& Direction) override;
	virtual bool ShootsBolts() const override { return false; }

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float SlideSpeed = 1250.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float SlideAcceleration = 9000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldHalfWidth = 185.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldDistance = 95.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldBend = 42.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield", meta = (Units = "deg"))
	float MaxTilt = 35.f;

	/** The light orb widens it, a blow narrows it. */
	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float WideScale = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float StrainScale = 0.62f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield", meta = (Units = "s"))
	float StrainTime = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	FLinearColor ShieldColor = FLinearColor(0.2f, 0.85f, 1.f);

private:
	void UpdateShield(float DeltaSeconds);

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Segments;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SegmentMaterials;

	UPROPERTY(Transient)
	TObjectPtr<AShieldField> Field;

	float SlideVelocity = 0.f;
	float Tilt = 0.f;
	float ShownHalfWidth = 185.f;
	float StrainLeft = 0.f;
	float ShieldFlash = 0.f;
	bool bHeld = false;
	bool bAutoRelease = false;

#if !UE_BUILD_SHIPPING
	/** -LWAutoShield: plays by itself, to check the mode can be finished. */
	FVector2D AutoPlay(float DeltaSeconds);
	float AutoOffset = 0.f;
	float AutoTimer = 0.f;
#endif
};
