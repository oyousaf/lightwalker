#pragma once

#include "CoreMinimal.h"
#include "Enemies/Dajjal.h"
#include "Enemies/Shaytaan.h"
#include "GameFramework/Actor.h"
#include "PathActors.generated.h"

class APathPawn;
class ULWParticleComponent;
class UStaticMeshComponent;

/** An orb of light: refills Lightwalker's lantern. Glows on its own, so it shows the way through the dark. */
UCLASS()
class LIGHTWALKER_API APathOrb : public AActor
{
	GENERATED_BODY()

public:
	APathOrb();

	virtual void Tick(float DeltaSeconds) override;

	void InitOrb(APathPawn* InPlayer) { Player = InPlayer; }

	/** Puts it back after Lightwalker returns to a checkpoint before it. It only scores the first time. */
	void Restore();

	bool IsTaken() const { return bTaken; }

	/** Called when it's collected; true the first time. */
	TFunction<void(APathOrb*, bool bFirst)> OnTaken;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Path")
	TObjectPtr<UStaticMeshComponent> Shell;

	UPROPERTY(VisibleAnywhere, Category = "Path")
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY(EditDefaultsOnly, Category = "Path")
	float CollectRadius = 75.f;

private:
	TWeakObjectPtr<APathPawn> Player;
	float Age = 0.f;
	bool bTaken = false;
	bool bScored = false;
};

/** A shaytaan on The Path: paces its stretch of ground, and charges when Lightwalker steps onto it. */
UCLASS()
class LIGHTWALKER_API APathShaytaan : public AShaytaan
{
	GENERATED_BODY()

public:
	void SetPatrol(float Left, float Right);

protected:
	virtual void UpdateMovement(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "Path")
	float PaceSpeed = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Path")
	float ChargeSpeed = 330.f;

	UPROPERTY(EditDefaultsOnly, Category = "Path")
	float NoticeDistance = 620.f;

private:
	float PatrolLeft = 0.f;
	float PatrolRight = 0.f;
	float Direction = -1.f;
};

/** A dajjal on The Path: hovers in place, rising and sinking, watching. */
UCLASS()
class LIGHTWALKER_API APathDajjal : public ADajjal
{
	GENERATED_BODY()

protected:
	virtual void UpdateMovement(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "Path")
	FVector2D HoverRange = FVector2D(140.f, 70.f);

private:
	FVector Home = FVector::ZeroVector;
	bool bHomeSet = false;
};
