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
 * Defaults (WASD, d-pad, fire and pause can be rebound in Settings > Controls, see ULightWalkerUserSettings):
 * Keyboard and mouse: WASD or arrow keys move. Space fires the way Lightwalker faces;
 * holding the left mouse button fires towards the cursor.
 * Gamepad: left stick or d-pad moves, right stick aims and fires (twin-stick). RT fires the way
 * Lightwalker faces. Start pauses.
 * On The Path: left and right run; Space, W, Up or A jumps (and again in the air); F, RT, the mouse or the right
 * stick fires.
 * Enter, Space or A restarts after a game over or a win; Esc, P or Start pauses. Esc or B goes back in menus.
 *
 * Also owns the menus (intro, main, pause, settings) and switches between menu and gameplay input.
 */
UCLASS()
class LIGHTWALKER_API ALightWalkerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALightWalkerPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	/** Launch: the intro screen, then the main menu. */
	void ShowIntro();
	void ShowMainMenu();

	/** Menu actions (called by the menu widgets). */
	void StartGameFromMenu();
	void CycleHero();
	FString GetHeroName() const;
	void OpenSettings();
	void OpenControls();
	void CloseControls();

	/** The Path's controls, with the player's own jump key. */
	FString GetPathHint() const;

	/** Rebuilds the input mappings from the saved bindings (after Settings > Controls changes them). */
	void ApplyBindings();
	void SetMenuPaused(bool bPaused);
	UFUNCTION(Exec)
	void RestartRun();
	UFUNCTION(Exec)
	void QuitToMenu();
	void QuitGame();

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
	void HandleJump(const FInputActionValue& Value);
	void HandleJumpReleased(const FInputActionValue& Value);
	void HandleMouseFire(const FInputActionValue& Value);
	void HandleMouseFireReleased(const FInputActionValue& Value);
	void HandleConfirm(const FInputActionValue& Value);
	void HandlePause(const FInputActionValue& Value);
	void HandleBack(const FInputActionValue& Value);

	/** Shows one menu (hiding the others) with UI input and the arrow cursor; null goes back to gameplay input. */
	void ShowMenu(class ULWMenuWidget* Menu);
	void CloseSettings();

	UPROPERTY(Transient)
	TObjectPtr<class ULWIntroScreen> IntroScreen;

	UPROPERTY(Transient)
	TObjectPtr<class ULWMainMenu> MainMenu;

	UPROPERTY(Transient)
	TObjectPtr<class ULWPauseMenu> PauseMenu;

	UPROPERTY(Transient)
	TObjectPtr<class ULWSettingsMenu> SettingsMenu;

	UPROPERTY(Transient)
	TObjectPtr<class ULWControlsMenu> ControlsMenu;

	/** The menu Settings returns to. */
	UPROPERTY(Transient)
	TObjectPtr<class ULWMenuWidget> SettingsReturn;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> BackAction;

	/** Where the cursor points on the plane the pawn moves in. */
	bool GetCursorTarget(const class ALightWalkerPawn& LightWalker, FVector& OutPoint) const;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MouseFireAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ConfirmAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PauseAction;

	bool bMouseFireHeld = false;
};
