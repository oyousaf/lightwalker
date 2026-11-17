#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LightWalkerPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Builds the Enhanced Input actions and bindings in code, so the game needs no input assets.
 *
 * Keyboard and mouse: WASD or arrow keys move. Space or Left Ctrl fires the way Lightwalker faces;
 * holding the left mouse button fires towards the cursor.
 * Gamepad: left stick or d-pad moves, right stick aims and fires (twin-stick). RT, RB or A fires the way
 * Lightwalker faces. Start pauses.
 * Enter, Space or A restarts after a game over or a win; Esc, P or Start pauses.
 */
UCLASS()
class LIGHTWALKER_API ALightWalkerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALightWalkerPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void BuildInput();

	void HandleMove(const FInputActionValue& Value);
	void HandleMoveReleased(const FInputActionValue& Value);
	void HandleAim(const FInputActionValue& Value);
	void HandleAimReleased(const FInputActionValue& Value);
	void HandleFire(const FInputActionValue& Value);
	void HandleFireReleased(const FInputActionValue& Value);
	void HandleMouseFire(const FInputActionValue& Value);
	void HandleMouseFireReleased(const FInputActionValue& Value);
	void HandleConfirm(const FInputActionValue& Value);
	void HandlePause(const FInputActionValue& Value);

	/** Where the cursor points on the arena plane at the pawn's height. */
	bool GetCursorOnPlane(float PlaneZ, FVector& OutPoint) const;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MouseFireAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ConfirmAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PauseAction;

	bool bMouseFireHeld = false;
};
