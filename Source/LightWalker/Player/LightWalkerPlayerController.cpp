#include "Player/LightWalkerPlayerController.h"

#include "Arena/ArenaGameMode.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Player/LightWalkerPawn.h"

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
	MouseFireAction = MakeAction(TEXT("IA_MouseFire"), EInputActionValueType::Boolean);
	ConfirmAction = MakeAction(TEXT("IA_Confirm"), EInputActionValueType::Boolean, true);
	PauseAction = MakeAction(TEXT("IA_Pause"), EInputActionValueType::Boolean, true);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Arena"));

	MapDirectionalKeys(MappingContext, MoveAction, EKeys::W, EKeys::A, EKeys::S, EKeys::D);
	MapDirectionalKeys(MappingContext, MoveAction, EKeys::Up, EKeys::Left, EKeys::Down, EKeys::Right);
	MapDirectionalKeys(MappingContext, MoveAction,
		EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Right);
	MapStick(MappingContext, MoveAction, EKeys::Gamepad_Left2D);

	MapStick(MappingContext, AimAction, EKeys::Gamepad_Right2D);

	MapKeys(MappingContext, FireAction, {
		EKeys::SpaceBar, EKeys::LeftControl,
		EKeys::Gamepad_RightTrigger, EKeys::Gamepad_RightShoulder, EKeys::Gamepad_FaceButton_Bottom });
	MapKeys(MappingContext, MouseFireAction, { EKeys::LeftMouseButton });

	MapKeys(MappingContext, ConfirmAction, { EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom });
	MapKeys(MappingContext, PauseAction, { EKeys::Escape, EKeys::P, EKeys::Gamepad_Special_Right });
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
	Input->BindAction(MouseFireAction, ETriggerEvent::Triggered, this, &ALightWalkerPlayerController::HandleMouseFire);
	Input->BindAction(MouseFireAction, ETriggerEvent::Completed, this, &ALightWalkerPlayerController::HandleMouseFireReleased);
	Input->BindAction(ConfirmAction, ETriggerEvent::Started, this, &ALightWalkerPlayerController::HandleConfirm);
	Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ALightWalkerPlayerController::HandlePause);
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
	if (bMouseFireHeld && GetCursorOnPlane(LightWalker->GetActorLocation().Z, Target))
	{
		LightWalker->SetCursorAim(true, Target);
	}
	else
	{
		LightWalker->SetCursorAim(false);
	}
}

bool ALightWalkerPlayerController::GetCursorOnPlane(float PlaneZ, FVector& OutPoint) const
{
	FVector Origin;
	FVector Direction;
	if (!DeprojectMousePositionToWorld(Origin, Direction) || FMath::IsNearlyZero(Direction.Z))
	{
		return false;
	}

	const float Distance = (PlaneZ - Origin.Z) / Direction.Z;
	if (Distance <= 0.f)
	{
		return false;
	}

	OutPoint = Origin + Direction * Distance;
	return true;
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
	if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
	{
		GameMode->RequestRestart();
	}
}

void ALightWalkerPlayerController::HandlePause(const FInputActionValue& Value)
{
	AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (!GameMode)
	{
		return;
	}

	// Start doubles as "play again" on the end screens; everywhere else it pauses.
	if (GameMode->IsRunOver())
	{
		GameMode->RequestRestart();
	}
	else
	{
		SetPause(!IsPaused());
	}
}
