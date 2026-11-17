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
 * Keyboard follows the 2014 layout: arrow keys move, WASD fires (two keys give the diagonals).
 * Gamepad is twin-stick: left stick moves, right stick aims and fires.
 */
UCLASS()
class LIGHTWALKER_API ALightWalkerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALightWalkerPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void BuildInput();

	void HandleMove(const FInputActionValue& Value);
	void HandleMoveReleased(const FInputActionValue& Value);
	void HandleAim(const FInputActionValue& Value);
	void HandleAimReleased(const FInputActionValue& Value);
	void HandleConfirm(const FInputActionValue& Value);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ConfirmAction;
};
