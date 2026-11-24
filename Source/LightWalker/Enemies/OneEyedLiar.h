#pragma once

#include "CoreMinimal.h"
#include "Enemies/Dajjal.h"
#include "OneEyedLiar.generated.h"

class AOneEyedLiar;
class UInstancedStaticMeshComponent;
class UPointLightComponent;

/** One of the small pyramids that guard the One-Eyed Liar: while any stands, it can't be hurt. */
UCLASS()
class LIGHTWALKER_API ALiarGuardian : public ADajjal
{
	GENERATED_BODY()

public:
	ALiarGuardian();

	void SetBoss(AOneEyedLiar* InBoss, float InAngle) { Boss = InBoss; Angle = InAngle; }

protected:
	virtual void UpdateMovement(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	float OrbitRadius = 430.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (Units = "deg"))
	float OrbitRate = 42.f;

private:
	TWeakObjectPtr<AOneEyedLiar> Boss;
	float Angle = 0.f;
	float Out = 0.f;
};

struct FBossBolt
{
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
};

/** The dark beam from the eye: a thin warning line first, then the beam itself, sweeping. */
struct FBossBeam
{
	enum class EState : uint8 { Off, Warning, Firing };
	EState State = EState::Off;
	float Age = 0.f;
	float Angle = 0.f;
	float SweepRate = 0.f;
};

/**
 * The boss, after wave 15: a giant dajjal that answers with dark bolts, a sweeping beam and whatever it can summon.
 *
 *   1. Drifts along the top: fans and rings of bolts, and shaytaan.
 *   2. Takes the centre behind four guardians. Break them all to open it for a while; then they return. Its beam sweeps.
 *   3. Comes after Lightwalker: spirals of bolts, the beam, and lesser dajjal.
 */
UCLASS()
class LIGHTWALKER_API AOneEyedLiar : public ADajjal
{
	GENERATED_BODY()

public:
	AOneEyedLiar();

	virtual void Tick(float DeltaSeconds) override;
	virtual void TakeHit() override;
	virtual void Kill() override;

	/** Starts its descent into the arena; it fights once set active. */
	void Arrive(const FVector& ArenaCentre, float Seconds);

	/** More health than usual (the endless arena, where it returns). Call before it fights. */
	void ScaleHealth(float Scale) { MaxHealth = FMath::RoundToInt(MaxHealth * Scale); Health = MaxHealth; }

	float GetHealth() const { return MaxHealth > 0 ? static_cast<float>(Health) / MaxHealth : 0.f; }
	int32 GetStage() const { return Stage; }

	/** The health at which stages 2 and 3 begin. The last stage is the longest. */
	static constexpr float StageTwoAt = 0.7f;
	static constexpr float StageThreeAt = 0.45f;
	bool IsGuarded() const { return Guardians.Num() > 0; }
	bool IsDying() const { return DyingTime >= 0.f; }

	/** Puts out the bolts and the beam (Lightwalker lost a life). */
	void ClearAttacks();

	const TArray<FBossBolt>& GetBolts() const { return Bolts; }
	const FBossBeam& GetBeam() const { return Beam; }
	float GetBossRadius() const { return Radius; }

	/** The game mode spawns what it calls for (and guardians count as its enemies too). */
	TFunction<void(int32 Shaytaan, int32 Dajjal)> OnSummon;
	TFunction<AEnemyBase*(TSubclassOf<AEnemyBase> Class, const FVector& Where)> OnSpawn;
	TFunction<void(int32 Stage)> OnStage;
	/** It's dying: the fight is won. And it's gone, at the end of its fall. */
	TFunction<void()> OnDefeated;
	TFunction<void()> OnGone;

protected:
	virtual void BeginPlay() override;
	virtual void UpdateMovement(float DeltaSeconds) override;
	virtual void UpdateLook(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	int32 MaxHealth = 200;

	/** Seconds it can be hurt in the second stage before its guardians return. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (Units = "s"))
	float OpenTime = 9.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	int32 GuardianCount = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	float BoltRadius = 24.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	float BeamHalfWidth = 48.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (Units = "s"))
	float BeamWarning = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (Units = "s"))
	float BeamDuration = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (Units = "s"))
	float DyingDuration = 3.4f;

	UPROPERTY(VisibleAnywhere, Category = "Boss")
	TObjectPtr<UInstancedStaticMeshComponent> BoltGlow;

	UPROPERTY(VisibleAnywhere, Category = "Boss")
	TObjectPtr<UInstancedStaticMeshComponent> BoltHeart;

	UPROPERTY(VisibleAnywhere, Category = "Boss")
	TObjectPtr<UStaticMeshComponent> BeamGlow;

	UPROPERTY(VisibleAnywhere, Category = "Boss")
	TObjectPtr<UStaticMeshComponent> BeamHeart;

	/** Swells in front of the eye before an attack. */
	UPROPERTY(VisibleAnywhere, Category = "Boss")
	TObjectPtr<UStaticMeshComponent> Charge;

	/** Circles it while it's guarded. */
	UPROPERTY(VisibleAnywhere, Category = "Boss")
	TObjectPtr<UInstancedStaticMeshComponent> GuardianRing;

	UPROPERTY(VisibleAnywhere, Category = "Boss")
	TObjectPtr<UPointLightComponent> Glow;

private:
	enum class EAttack : uint8 { Fan, Ring, Summon, Beam, Spiral };

	void EnterStage(int32 NewStage);
	void RaiseGuardians();
	void ChooseAttack();
	void BeginAttack(EAttack Attack);
	void UpdateAttack(float DeltaSeconds);
	void UpdateBolts(float DeltaSeconds, bool bCanHurt);
	void UpdateBeam(float DeltaSeconds, bool bCanHurt);
	void UpdateDying(float DeltaSeconds);
	void AddBolt(float AngleDegrees, float Speed);
	float AngleToPlayer() const;
	FVector2D GetEye() const;

	TArray<FBossBolt> Bolts;
	/** How many bolts were drawn last frame. */
	int32 BoltsShown = 0;
	FBossBeam Beam;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALiarGuardian>> Guardians;

	FVector Centre = FVector::ZeroVector;
	int32 Health = 200;
	int32 Stage = 1;
	int32 AttackIndex = 0;
	EAttack Current = EAttack::Fan;
	/** Counts down to the attack (charging), then through it. */
	float ChargeLeft = -1.f;
	float AttackWait = 2.f;
	float SpiralLeft = 0.f;
	float SpiralTimer = 0.f;
	float SpiralAngle = 0.f;
	float StageAge = 0.f;
	float OpenLeft = 0.f;
	float ArriveLeft = 0.f;
	float ArriveTime = 1.f;
	float DyingTime = -1.f;
	float DyingBurst = 0.f;
	float GlowPulse = 0.f;
	bool bFighting = false;
	bool bRingShown = false;
};
