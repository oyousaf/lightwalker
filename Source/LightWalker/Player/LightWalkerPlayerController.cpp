#include "Player/LightWalkerPlayerController.h"

#include "Core/LWGameMode.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Core/LWFeel.h"
#include "Core/LWScoreboard.h"
#include "Core/LWSound.h"
#include "Core/LightWalkerUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/LightWalkerPawn.h"
#include "UI/LWMenuWidgets.h"
#include "UI/LWScoreWidgets.h"

namespace
{
	/** Directional keys feed a 2D action: right = +X, left = -X, up = +Y, down = -Y. */
	enum class EKeyAxis : uint8 { Right, Left, Up, Down };

	void MapDirectionalKey(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key, EKeyAxis Axis)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);

		if (Axis == EKeyAxis::Up || Axis == EKeyAxis::Down)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (Axis == EKeyAxis::Left || Axis == EKeyAxis::Down)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
	}

	/** Four keys (or d-pad buttons) as one 2D direction. */
	void MapDirectionalKeys(UInputMappingContext* Context, const UInputAction* Action,
		const FKey& Up, const FKey& Left, const FKey& Down, const FKey& Right)
	{
		MapDirectionalKey(Context, Action, Up, EKeyAxis::Up);
		MapDirectionalKey(Context, Action, Left, EKeyAxis::Left);
		MapDirectionalKey(Context, Action, Down, EKeyAxis::Down);
		MapDirectionalKey(Context, Action, Right, EKeyAxis::Right);
	}

	void MapStick(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		UInputModifierDeadZone* DeadZone = NewObject<UInputModifierDeadZone>(Context);
		DeadZone->LowerThreshold = 0.2f;
		Mapping.Modifiers.Add(DeadZone);
	}

	void MapKeys(UInputMappingContext* Context, const UInputAction* Action, std::initializer_list<FKey> Keys)
	{
		for (const FKey& Key : Keys)
		{
			Context->MapKey(Action, Key);
		}
	}
}

ALightWalkerPlayerController::ALightWalkerPlayerController()
{
	// The arena camera is the view target, not the pawn.
	bAutoManageActiveCameraTarget = false;

	// A crosshair cursor for mouse aiming.
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ALightWalkerPlayerController::BuildInput()
{
	if (MappingContext)
	{
		return;
	}

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type, bool bWhenPaused = false)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		Action->bTriggerWhenPaused = bWhenPaused;
		return Action;
	};

	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	AimAction = MakeAction(TEXT("IA_Aim"), EInputActionValueType::Axis2D);
	FireAction = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	MouseFireAction = MakeAction(TEXT("IA_MouseFire"), EInputActionValueType::Boolean);
	ConfirmAction = MakeAction(TEXT("IA_Confirm"), EInputActionValueType::Boolean, true);
	PauseAction = MakeAction(TEXT("IA_Pause"), EInputActionValueType::Boolean, true);
	BackAction = MakeAction(TEXT("IA_Back"), EInputActionValueType::Boolean, true);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Arena"));
	ApplyBindings();
}

void ALightWalkerPlayerController::ApplyBindings()
{
	if (!MappingContext)
	{
		return;
	}
	MappingContext->UnmapAll();
	const ALWGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALWGameMode>() : nullptr;
	const bool bJumping = GameMode && GameMode->UsesJump();

	// The player's bindings (Settings > Controls), keyboard and gamepad.
	const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
	for (const bool bGamepad : { false, true })
	{
		auto Bound = [Settings, bGamepad](ELWBinding Binding)
		{
			return Settings ? Settings->GetBinding(Binding, bGamepad) : ULightWalkerUserSettings::GetDefaultBinding(Binding, bGamepad);
		};
		MapDirectionalKeys(MappingContext, MoveAction,
			Bound(ELWBinding::MoveUp), Bound(ELWBinding::MoveLeft), Bound(ELWBinding::MoveDown), Bound(ELWBinding::MoveRight));
		MapKeys(MappingContext, PauseAction, { Bound(ELWBinding::Pause) });

		FKey Fire = Bound(ELWBinding::Fire);
		if (bJumping)
		{
			// On The Path, jump wins a key it shares with fire (the space bar by default), and fire moves to F.
			MapKeys(MappingContext, JumpAction, { Bound(ELWBinding::Jump) });
			if (Fire == Bound(ELWBinding::Jump))
			{
				Fire = bGamepad ? EKeys::Gamepad_RightTrigger : EKeys::F;
			}
			if (!bGamepad && Bound(ELWBinding::MoveUp) != Bound(ELWBinding::Jump))
			{
				MapKeys(MappingContext, JumpAction, { Bound(ELWBinding::MoveUp), EKeys::Up });
			}
		}
		MapKeys(MappingContext, FireAction, { Fire });
	}

	// Fixed: the arrow keys and sticks always move and aim, the mouse always aims, Esc always pauses.
	MapDirectionalKeys(MappingContext, MoveAction, EKeys::Up, EKeys::Left, EKeys::Down, EKeys::Right);
	MapStick(MappingContext, MoveAction, EKeys::Gamepad_Left2D);
	MapStick(MappingContext, AimAction, EKeys::Gamepad_Right2D);
	MapKeys(MappingContext, MouseFireAction, { EKeys::LeftMouseButton });
	MapKeys(MappingContext, PauseAction, { EKeys::Escape });
	MapKeys(MappingContext, ConfirmAction, { EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom });
	MapKeys(MappingContext, BackAction, { EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right });

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetLocalPlayer() ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) : nullptr)
	{
		Subsystem->RequestRebuildControlMappings();
	}
}

void ALightWalkerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	BuildInput();

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALightWalkerPlayerController::HandleMove);
	Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &ALightWalkerPlayerController::HandleMoveReleased);
	Input->BindAction(AimAction, ETriggerEvent::Triggered, this, &ALightWalkerPlayerController::HandleAim);
	Input->BindAction(AimAction, ETriggerEvent::Completed, this, &ALightWalkerPlayerController::HandleAimReleased);
	Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &ALightWalkerPlayerController::HandleFire);
	Input->BindAction(FireAction, ETriggerEvent::Completed, this, &ALightWalkerPlayerController::HandleFireReleased);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ALightWalkerPlayerController::HandleJump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ALightWalkerPlayerController::HandleJumpReleased);
	Input->BindAction(MouseFireAction, ETriggerEvent::Triggered, this, &ALightWalkerPlayerController::HandleMouseFire);
	Input->BindAction(MouseFireAction, ETriggerEvent::Completed, this, &ALightWalkerPlayerController::HandleMouseFireReleased);
	Input->BindAction(ConfirmAction, ETriggerEvent::Started, this, &ALightWalkerPlayerController::HandleConfirm);
	Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ALightWalkerPlayerController::HandlePause);
	Input->BindAction(BackAction, ETriggerEvent::Started, this, &ALightWalkerPlayerController::HandleBack);
}

void ALightWalkerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	BuildInput();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockInFullscreen);
	SetInputMode(InputMode);
}

void ALightWalkerPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>();
	if (!LightWalker)
	{
		return;
	}

	FVector Target;
	if (bMouseFireHeld && GetCursorTarget(*LightWalker, Target))
	{
		LightWalker->SetCursorAim(true, Target);
	}
	else
	{
		LightWalker->SetCursorAim(false);
	}
}

bool ALightWalkerPlayerController::GetCursorTarget(const ALightWalkerPawn& LightWalker, FVector& OutPoint) const
{
	FVector Origin;
	FVector Direction;
	return DeprojectMousePositionToWorld(Origin, Direction) && LightWalker.ProjectCursor(Origin, Direction, OutPoint);
}

void ALightWalkerPlayerController::HandleMove(const FInputActionValue& Value)
{
	if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
	{
		LightWalker->SetMoveInput(Value.Get<FVector2D>());
	}
}

void ALightWalkerPlayerController::HandleMoveReleased(const FInputActionValue& Value)
{
	if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
	{
		LightWalker->SetMoveInput(FVector2D::ZeroVector);
	}
}

void ALightWalkerPlayerController::HandleAim(const FInputActionValue& Value)
{
	if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
	{
		LightWalker->SetAimInput(Value.Get<FVector2D>());
	}
}

void ALightWalkerPlayerController::HandleAimReleased(const FInputActionValue& Value)
{
	if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
	{
		LightWalker->SetAimInput(FVector2D::ZeroVector);
	}
}

void ALightWalkerPlayerController::HandleFire(const FInputActionValue& Value)
{
	if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
	{
		LightWalker->SetFireHeld(true);
	}
}

void ALightWalkerPlayerController::HandleFireReleased(const FInputActionValue& Value)
{
	if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
	{
		LightWalker->SetFireHeld(false);
	}
}

void ALightWalkerPlayerController::HandleJump(const FInputActionValue& Value)
{
	if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
	{
		LightWalker->SetJumpHeld(true);
	}
}

void ALightWalkerPlayerController::HandleJumpReleased(const FInputActionValue& Value)
{
	if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
	{
		LightWalker->SetJumpHeld(false);
	}
}

void ALightWalkerPlayerController::HandleMouseFire(const FInputActionValue& Value)
{
	bMouseFireHeld = true;
}

void ALightWalkerPlayerController::HandleMouseFireReleased(const FInputActionValue& Value)
{
	bMouseFireHeld = false;
}

void ALightWalkerPlayerController::HandleConfirm(const FInputActionValue& Value)
{
	if (ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>())
	{
		GameMode->RequestRestart();
	}
}

void ALightWalkerPlayerController::HandlePause(const FInputActionValue& Value)
{
	ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>();
	if (!GameMode)
	{
		return;
	}

	if (IntroScreen && IntroScreen->IsInViewport())
	{
		IntroScreen->Skip();
	}
	else if (ControlsMenu && ControlsMenu->IsInViewport())
	{
		CloseControls();
	}
	else if (SettingsMenu && SettingsMenu->IsInViewport())
	{
		CloseSettings();
	}
	else if (IsRunEndShown())
	{
		// The buttons decide.
	}
	else if (Leaderboard && Leaderboard->IsInViewport())
	{
		CloseLeaderboard();
	}
	else if (GameMode->IsInMenu())
	{
		// Nothing to pause on the main menu.
	}
	else if (GameMode->IsRunOver())
	{
		// Start skips the wait for the leaderboard.
		GameMode->RequestRestart();
	}
	else
	{
		SetMenuPaused(!IsPaused());
	}
}

void ALightWalkerPlayerController::HandleBack(const FInputActionValue& Value)
{
	if (ControlsMenu && ControlsMenu->IsInViewport())
	{
		CloseControls();
	}
	else if (SettingsMenu && SettingsMenu->IsInViewport())
	{
		CloseSettings();
	}
	else if (Leaderboard && Leaderboard->IsInViewport() && !IsRunEndShown())
	{
		CloseLeaderboard();
	}
	else if (IsPaused())
	{
		SetMenuPaused(false);
	}
}

void ALightWalkerPlayerController::ShowMenu(ULWMenuWidget* Menu)
{
	for (ULWMenuWidget* Other : { (ULWMenuWidget*)IntroScreen.Get(), (ULWMenuWidget*)MainMenu.Get(), (ULWMenuWidget*)PauseMenu.Get(), (ULWMenuWidget*)SettingsMenu.Get(), (ULWMenuWidget*)ControlsMenu.Get(), (ULWMenuWidget*)NameEntry.Get(), (ULWMenuWidget*)Leaderboard.Get() })
	{
		if (Other && Other != Menu && Other->IsInViewport())
		{
			Other->RemoveFromParent();
		}
	}

	if (!Menu)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockInFullscreen);
		SetInputMode(InputMode);
		CurrentMouseCursor = EMouseCursor::Crosshairs;
		return;
	}

	if (!Menu->IsInViewport())
	{
		Menu->AddToViewport(10);
	}
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(Menu->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	CurrentMouseCursor = EMouseCursor::Default;

	// Clear held inputs so nothing carries from the menu into play (or back).
	if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
	{
		LightWalker->SetMoveInput(FVector2D::ZeroVector);
		LightWalker->SetAimInput(FVector2D::ZeroVector);
		LightWalker->SetFireHeld(false);
		LightWalker->SetJumpHeld(false);
	}
	bMouseFireHeld = false;

	if (Menu != IntroScreen)
	{
		Menu->FocusButton();
	}
}

void ALightWalkerPlayerController::ShowIntro()
{
	if (!IntroScreen)
	{
		IntroScreen = CreateWidget<ULWIntroScreen>(this, ULWIntroScreen::StaticClass());
		IntroScreen->OnFinished = [this]() { ShowMainMenu(); };
	}
	ShowMenu(IntroScreen);
	IntroScreen->SetKeyboardFocus();
	IntroScreen->SetUserFocus(this);
}

void ALightWalkerPlayerController::ShowMainMenu()
{
	if (!MainMenu)
	{
		MainMenu = CreateWidget<ULWMainMenu>(this, ULWMainMenu::StaticClass());
	}
	ShowMenu(MainMenu);
}

void ALightWalkerPlayerController::StartGameFromMenu(ERunMode Mode)
{
	if (ULightWalkerGameInstance* Run = GetGameInstance<ULightWalkerGameInstance>())
	{
		Run->SetRunMode(Mode);
	}
	ULWSoundSubsystem::Play(this, ELWSound::UIPlay);
	ShowMenu(nullptr);
	if (ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>())
	{
		GameMode->StartFromMenu();
	}
}

void ALightWalkerPlayerController::OpenLeaderboard()
{
	if (!Leaderboard)
	{
		Leaderboard = CreateWidget<ULWLeaderboard>(this, ULWLeaderboard::StaticClass());
	}
	Leaderboard->OnBack = [this]() { CloseLeaderboard(); };
	Leaderboard->Begin(ERunMode::Endless, -1, false, 0);
	ShowMenu(Leaderboard);
	Leaderboard->FocusButton(2);
}

void ALightWalkerPlayerController::CloseLeaderboard()
{
	ShowMainMenu();
}

bool ALightWalkerPlayerController::IsRunEndShown() const
{
	const ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>();
	return GameMode && GameMode->IsRunOver() && ((NameEntry && NameEntry->IsInViewport()) || (Leaderboard && Leaderboard->IsInViewport()));
}

void ALightWalkerPlayerController::ShowRunEnd()
{
	const ULightWalkerGameInstance* Run = GetGameInstance<ULightWalkerGameInstance>();
	ULWScoreboard* Scores = ULWScoreboard::Get(this);
	if (!Run || !Scores || IsRunEndShown())
	{
		return;
	}
	if (!Scores->Qualifies(Run->GetRunMode(), Run->GetScore()))
	{
		ShowRunBoard(-1);
		return;
	}

	if (!NameEntry)
	{
		NameEntry = CreateWidget<ULWNameEntry>(this, ULWNameEntry::StaticClass());
		NameEntry->OnDone = [this](const FString& Name)
		{
			const ULightWalkerGameInstance* Ended = GetGameInstance<ULightWalkerGameInstance>();
			const ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>();
			ULWScoreboard* Board = ULWScoreboard::Get(this);
			ShowRunBoard(Board->Add(Ended->GetRunMode(), Name, Ended->GetScore(), GameMode ? GameMode->GetReached() : FString()));
		};
	}
	NameEntry->Begin(Run->GetScore(), Scores->GetLastName());
	ShowMenu(NameEntry);
}

void ALightWalkerPlayerController::ShowRunBoard(int32 Place)
{
	const ULightWalkerGameInstance* Run = GetGameInstance<ULightWalkerGameInstance>();
	if (!Leaderboard)
	{
		Leaderboard = CreateWidget<ULWLeaderboard>(this, ULWLeaderboard::StaticClass());
	}
	Leaderboard->OnBack = [this]() { QuitToMenu(); };
	Leaderboard->OnPlayAgain = [this]() { RestartRun(); };
	Leaderboard->Begin(Run->GetRunMode(), Place, true, Run->GetScore());
	ShowMenu(Leaderboard);
	Leaderboard->FocusButton(0);
}

void ALightWalkerPlayerController::CycleHero()
{
	if (ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get())
	{
		Settings->HeroIndex = (Settings->HeroIndex + 1) % 2;
		Settings->SaveSettings();
		if (ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>())
		{
			LightWalker->SetHero(Settings->HeroIndex);
		}
	}
}

FString ALightWalkerPlayerController::GetHeroName() const
{
	const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
	const int32 Index = Settings ? Settings->HeroIndex : 0;
	const ALightWalkerPawn* LightWalker = GetPawn<ALightWalkerPawn>();
	if (Index == 1)
	{
		return LightWalker && LightWalker->HasHeroModel(1) ? TEXT("FEMALE") : TEXT("FEMALE (COMING SOON)");
	}
	return TEXT("MALE");
}

FString ALightWalkerPlayerController::GetPathHint() const
{
	const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
	auto Name = [Settings](ELWBinding Binding, bool bGamepad)
	{
		return ULightWalkerUserSettings::GetKeyLabel(Settings ? Settings->GetBinding(Binding, bGamepad) : ULightWalkerUserSettings::GetDefaultBinding(Binding, bGamepad));
	};
	const bool bShared = Name(ELWBinding::Fire, false) == Name(ELWBinding::Jump, false);
	return FString::Printf(TEXT("%s TO JUMP, AGAIN IN THE AIR     %s OR CLICK TO FIRE     GAMEPAD: %s TO JUMP, %s TO FIRE"),
		*Name(ELWBinding::Jump, false), bShared ? TEXT("F") : *Name(ELWBinding::Fire, false), *Name(ELWBinding::Jump, true), *Name(ELWBinding::Fire, true));
}

FString ALightWalkerPlayerController::GetShieldHint() const
{
	const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
	auto Fire = [Settings](bool bGamepad)
	{
		return ULightWalkerUserSettings::GetKeyLabel(Settings ? Settings->GetBinding(ELWBinding::Fire, bGamepad) : ULightWalkerUserSettings::GetDefaultBinding(ELWBinding::Fire, bGamepad));
	};
	return FString::Printf(TEXT("%s OR %s RELEASES THE BEAM     LEFT AND RIGHT MOVE     HOLD THE MOUSE OR RIGHT STICK TO TILT"), *Fire(false), *Fire(true));
}

void ALightWalkerPlayerController::OpenSettings()
{
	if (!SettingsMenu)
	{
		SettingsMenu = CreateWidget<ULWSettingsMenu>(this, ULWSettingsMenu::StaticClass());
		SettingsMenu->OnBack = [this]() { CloseSettings(); };
	}
	SettingsReturn = (PauseMenu && PauseMenu->IsInViewport()) ? (ULWMenuWidget*)PauseMenu.Get() : (ULWMenuWidget*)MainMenu.Get();
	ShowMenu(SettingsMenu);
}

void ALightWalkerPlayerController::CloseSettings()
{
	if (ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get())
	{
		Settings->SaveSettings();
	}
	ShowMenu(SettingsReturn ? SettingsReturn.Get() : (ULWMenuWidget*)MainMenu.Get());
}

void ALightWalkerPlayerController::OpenControls()
{
	if (!ControlsMenu)
	{
		ControlsMenu = CreateWidget<ULWControlsMenu>(this, ULWControlsMenu::StaticClass());
	}
	ShowMenu(ControlsMenu);
}

void ALightWalkerPlayerController::CloseControls()
{
	if (ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get())
	{
		Settings->SaveSettings();
	}
	ShowMenu(SettingsMenu);
}

void ALightWalkerPlayerController::SetMenuPaused(bool bPaused)
{
	if (ULWFeelSubsystem* Feel = GetGameInstance()->GetSubsystem<ULWFeelSubsystem>())
	{
		Feel->CancelHitStop();
	}
	SetPause(bPaused);
	if (bPaused)
	{
		if (!PauseMenu)
		{
			PauseMenu = CreateWidget<ULWPauseMenu>(this, ULWPauseMenu::StaticClass());
		}
		ShowMenu(PauseMenu);
	}
	else
	{
		ShowMenu(nullptr);
	}
}

void ALightWalkerPlayerController::RestartRun()
{
	SetMenuPaused(false);
	if (ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>())
	{
		GameMode->RestartRun();
	}
}

void ALightWalkerPlayerController::QuitToMenu()
{
	SetPause(false);
	if (ALWGameMode* GameMode = GetWorld()->GetAuthGameMode<ALWGameMode>())
	{
		GameMode->ReturnToMenu();
	}
	ShowMainMenu();
}

void ALightWalkerPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
