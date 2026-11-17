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

	void MapStick(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		UInputModifierDeadZone* DeadZone = NewObject<UInputModifierDeadZone>(Context);
		DeadZone->LowerThreshold = 0.2f;
		Mapping.Modifiers.Add(DeadZone);
	}
}

ALightWalkerPlayerController::ALightWalkerPlayerController()
{
	// The arena camera is the view target, not the pawn.
	bAutoManageActiveCameraTarget = false;
	bShowMouseCursor = false;
}

void ALightWalkerPlayerController::BuildInput()
{
	if (MappingContext)
	{
		return;
	}

	MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	AimAction = NewObject<UInputAction>(this, TEXT("IA_Aim"));
	AimAction->ValueType = EInputActionValueType::Axis2D;

	ConfirmAction = NewObject<UInputAction>(this, TEXT("IA_Confirm"));
	ConfirmAction->ValueType = EInputActionValueType::Boolean;

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Arena"));

	MapDirectionalKey(MappingContext, MoveAction, EKeys::Right, EKeyAxis::Right);
	MapDirectionalKey(MappingContext, MoveAction, EKeys::Left, EKeyAxis::Left);
	MapDirectionalKey(MappingContext, MoveAction, EKeys::Up, EKeyAxis::Up);
	MapDirectionalKey(MappingContext, MoveAction, EKeys::Down, EKeyAxis::Down);
	MapStick(MappingContext, MoveAction, EKeys::Gamepad_Left2D);

	MapDirectionalKey(MappingContext, AimAction, EKeys::D, EKeyAxis::Right);
	MapDirectionalKey(MappingContext, AimAction, EKeys::A, EKeyAxis::Left);
	MapDirectionalKey(MappingContext, AimAction, EKeys::W, EKeyAxis::Up);
	MapDirectionalKey(MappingContext, AimAction, EKeys::S, EKeyAxis::Down);
	MapStick(MappingContext, AimAction, EKeys::Gamepad_Right2D);

	MappingContext->MapKey(ConfirmAction, EKeys::Enter);
	MappingContext->MapKey(ConfirmAction, EKeys::SpaceBar);
	MappingContext->MapKey(ConfirmAction, EKeys::Gamepad_FaceButton_Bottom);
	MappingContext->MapKey(ConfirmAction, EKeys::Gamepad_Special_Right);
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
	Input->BindAction(ConfirmAction, ETriggerEvent::Started, this, &ALightWalkerPlayerController::HandleConfirm);
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

	SetInputMode(FInputModeGameOnly());
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

void ALightWalkerPlayerController::HandleConfirm(const FInputActionValue& Value)
{
	if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
	{
		GameMode->RequestRestart();
	}
}
