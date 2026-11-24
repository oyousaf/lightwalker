#include "Core/LightWalkerUserSettings.h"

#include "Engine/Engine.h"
#include "InputCoreTypes.h"

namespace
{
	constexpr int32 CurrentSettingsVersion = 1;
	constexpr float DefaultFrameRateLimit = 60.f;
}

ULightWalkerUserSettings* ULightWalkerUserSettings::Get()
{
	return GEngine ? Cast<ULightWalkerUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void ULightWalkerUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	FrameRateLimit = DefaultFrameRateLimit;
	bUseVSync = true;
	ResetBindings();
	LightWalkerSettingsVersion = CurrentSettingsVersion;
}

void ULightWalkerUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);

	// Settings saved before the 60 fps default existed were uncapped; bring them in line once.
	if (LightWalkerSettingsVersion < 1)
	{
		FrameRateLimit = DefaultFrameRateLimit;
	}
	LightWalkerSettingsVersion = CurrentSettingsVersion;
}

FKey ULightWalkerUserSettings::GetDefaultBinding(ELWBinding Binding, bool bGamepad)
{
	switch (Binding)
	{
	case ELWBinding::MoveUp:    return bGamepad ? EKeys::Gamepad_DPad_Up : EKeys::W;
	case ELWBinding::MoveLeft:  return bGamepad ? EKeys::Gamepad_DPad_Left : EKeys::A;
	case ELWBinding::MoveDown:  return bGamepad ? EKeys::Gamepad_DPad_Down : EKeys::S;
	case ELWBinding::MoveRight: return bGamepad ? EKeys::Gamepad_DPad_Right : EKeys::D;
	case ELWBinding::Fire:      return bGamepad ? EKeys::Gamepad_RightTrigger : EKeys::SpaceBar;
	case ELWBinding::Pause:     return bGamepad ? EKeys::Gamepad_Special_Right : EKeys::P;
	case ELWBinding::Jump:      return bGamepad ? EKeys::Gamepad_FaceButton_Bottom : EKeys::SpaceBar;
	default:                    return EKeys::Invalid;
	}
}

FKey ULightWalkerUserSettings::GetBinding(ELWBinding Binding, bool bGamepad) const
{
	const TArray<FKey>& List = bGamepad ? GamepadBindings : KeyboardBindings;
	const int32 Index = static_cast<int32>(Binding);
	if (List.IsValidIndex(Index) && List[Index].IsValid() && CanBind(List[Index]) && List[Index].IsGamepadKey() == bGamepad)
	{
		return List[Index];
	}
	return GetDefaultBinding(Binding, bGamepad);
}

bool ULightWalkerUserSettings::SetBinding(ELWBinding Binding, const FKey& Key)
{
	if (!CanBind(Key) || Binding >= ELWBinding::Count)
	{
		return false;
	}

	const bool bGamepad = Key.IsGamepadKey();
	TArray<FKey> Current;
	for (int32 Index = 0; Index < static_cast<int32>(ELWBinding::Count); ++Index)
	{
		Current.Add(GetBinding(static_cast<ELWBinding>(Index), bGamepad));
	}

	// Whatever had the key takes over this control's old one.
	const int32 Target = static_cast<int32>(Binding);
	const FKey Old = Current[Target];
	const int32 Jump = static_cast<int32>(ELWBinding::Jump);
	const int32 Fire = static_cast<int32>(ELWBinding::Fire);
	for (int32 Index = 0; Index < Current.Num(); ++Index)
	{
		const bool bMayShare = (Index == Jump && Target == Fire) || (Index == Fire && Target == Jump);
		if (Current[Index] == Key && !bMayShare)
		{
			Current[Index] = Old;
		}
	}
	Current[Target] = Key;
	(bGamepad ? GamepadBindings : KeyboardBindings) = MoveTemp(Current);
	return true;
}

void ULightWalkerUserSettings::ResetBindings()
{
	KeyboardBindings.Reset();
	GamepadBindings.Reset();
}

bool ULightWalkerUserSettings::CanBind(const FKey& Key)
{
	if (!Key.IsValid() || Key.IsMouseButton() || Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D() || Key.IsTouch())
	{
		return false;
	}
	// The sticks' digital directions would trigger from aiming and moving.
	if (Key.IsGamepadKey() && Key.GetFName().ToString().Contains(TEXT("Stick_")))
	{
		return false;
	}
	return Key != EKeys::Escape && Key != EKeys::Up && Key != EKeys::Down && Key != EKeys::Left && Key != EKeys::Right;
}

FString ULightWalkerUserSettings::GetKeyLabel(const FKey& Key)
{
	struct FShortName { FKey Key; const TCHAR* Label; };
	static const FShortName ShortNames[] = {
		{ EKeys::Gamepad_FaceButton_Bottom, TEXT("A") },
		{ EKeys::Gamepad_FaceButton_Right, TEXT("B") },
		{ EKeys::Gamepad_FaceButton_Left, TEXT("X") },
		{ EKeys::Gamepad_FaceButton_Top, TEXT("Y") },
		{ EKeys::Gamepad_LeftShoulder, TEXT("LB") },
		{ EKeys::Gamepad_RightShoulder, TEXT("RB") },
		{ EKeys::Gamepad_LeftTrigger, TEXT("LT") },
		{ EKeys::Gamepad_RightTrigger, TEXT("RT") },
		{ EKeys::Gamepad_LeftThumbstick, TEXT("L3") },
		{ EKeys::Gamepad_RightThumbstick, TEXT("R3") },
		{ EKeys::Gamepad_DPad_Up, TEXT("D-PAD UP") },
		{ EKeys::Gamepad_DPad_Down, TEXT("D-PAD DOWN") },
		{ EKeys::Gamepad_DPad_Left, TEXT("D-PAD LEFT") },
		{ EKeys::Gamepad_DPad_Right, TEXT("D-PAD RIGHT") },
		{ EKeys::Gamepad_Special_Right, TEXT("START") },
		{ EKeys::Gamepad_Special_Left, TEXT("VIEW") },
	};
	for (const FShortName& Name : ShortNames)
	{
		if (Name.Key == Key)
		{
			return Name.Label;
		}
	}
	return Key.GetDisplayName(false).ToString().ToUpper();
}

FString ULightWalkerUserSettings::GetBindingLabel(ELWBinding Binding)
{
	switch (Binding)
	{
	case ELWBinding::MoveUp:    return TEXT("MOVE UP");
	case ELWBinding::MoveLeft:  return TEXT("MOVE LEFT");
	case ELWBinding::MoveDown:  return TEXT("MOVE DOWN");
	case ELWBinding::MoveRight: return TEXT("MOVE RIGHT");
	case ELWBinding::Fire:      return TEXT("FIRE");
	case ELWBinding::Pause:     return TEXT("PAUSE");
	case ELWBinding::Jump:      return TEXT("JUMP (THE PATH)");
	default:                    return FString();
	}
}
