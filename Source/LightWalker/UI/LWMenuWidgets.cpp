#include "UI/LWMenuWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Brushes/SlateImageBrush.h"
#include "Misc/Paths.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/LightWalkerUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/LightWalkerPlayerController.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor Glow(0.55f, 0.95f, 1.f);
	const FLinearColor Muted(0.5f, 0.62f, 0.75f);
	const FLinearColor Warm(1.f, 0.88f, 0.6f);
	const FLinearColor IdleFill(0.f, 0.02f, 0.05f, 0.45f);
	const FLinearColor ActiveFill(0.1f, 0.75f, 0.8f, 0.35f);

	FButtonStyle MakeButtonStyle()
	{
		// White rounded boxes with a thin cyan outline; the fill colour comes from the button's background tint.
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(8.f, 8.f, 8.f, 8.f), FSlateColor(FLinearColor(0.35f, 0.9f, 1.f, 0.55f)), 1.5f);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;

		FButtonStyle Style;
		Style.SetNormal(Brush).SetHovered(Brush).SetPressed(Brush).SetDisabled(Brush);
		Style.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
		return Style;
	}

	FString QualityName(int32 Level)
	{
		switch (Level)
		{
		case 0: return TEXT("LOW");
		case 1: return TEXT("MEDIUM");
		case 2: return TEXT("HIGH");
		case 3: return TEXT("EPIC");
		default: return TEXT("CUSTOM");
		}
	}

	FString WindowName(EWindowMode::Type Mode)
	{
		switch (Mode)
		{
		case EWindowMode::Fullscreen: return TEXT("FULLSCREEN");
		case EWindowMode::WindowedFullscreen: return TEXT("BORDERLESS");
		default: return TEXT("WINDOWED");
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Base

TSharedRef<SWidget> ULWMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		Build(Root);
	}
	return Super::RebuildWidget();
}

ALightWalkerPlayerController* ULWMenuWidget::GetLWController() const
{
	return Cast<ALightWalkerPlayerController>(GetOwningPlayer());
}

UTextBlock* ULWMenuWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color, int32 LetterSpacing)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", Size);
	Font.LetterSpacing = LetterSpacing;
	Block->SetFont(Font);
	Block->SetText(FText::FromString(Text));
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetShadowOffset(FVector2D(2.f, 2.f));
	Block->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Block->SetJustification(ETextJustify::Center);
	return Block;
}

UButton* ULWMenuWidget::AddButton(UPanelWidget* Parent, const FString& Label, TFunction<void()> OnClick)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetStyle(MakeButtonStyle());
	Button->SetBackgroundColor(IdleFill);

	UTextBlock* Text = MakeText(Label, 22, Muted, 120);
	if (UButtonSlot* PanelSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
	{
		PanelSlot->SetPadding(FMargin(28.f, 12.f));
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
	}

	ULWClickRelay* Relay = NewObject<ULWClickRelay>(this);
	Relay->Callback = MoveTemp(OnClick);
	Button->OnClicked.AddDynamic(Relay, &ULWClickRelay::HandleClicked);
	Relays.Add(Relay);

	UPanelSlot* PanelSlot = Parent->AddChild(Button);
	if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(PanelSlot))
	{
		VerticalSlot->SetPadding(FMargin(0.f, 6.f));
		VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	Focusables.Add({ Button, Button, Text });
	return Button;
}

void ULWMenuWidget::AddSlider(UPanelWidget* Parent, const FString& Label, float Value, TFunction<void(float)> OnChange)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

	UTextBlock* Text = MakeText(Label, 20, Muted, 120);
	Text->SetJustification(ETextJustify::Left);
	if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Text))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetPadding(FMargin(8.f, 0.f));
	}

	USlider* Slider = WidgetTree->ConstructWidget<USlider>();
	Slider->SetValue(Value);
	Slider->SetStepSize(0.05f);
	Slider->SetSliderBarColor(FLinearColor(0.2f, 0.3f, 0.4f));
	Slider->SetSliderHandleColor(Glow);
	USizeBox* SliderBox = WidgetTree->ConstructWidget<USizeBox>();
	SliderBox->SetWidthOverride(300.f);
	SliderBox->AddChild(Slider);
	if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(SliderBox))
	{
		SliderSlot->SetVerticalAlignment(VAlign_Center);
		SliderSlot->SetPadding(FMargin(8.f, 0.f));
	}

	ULWSliderRelay* Relay = NewObject<ULWSliderRelay>(this);
	Relay->Callback = MoveTemp(OnChange);
	Slider->OnValueChanged.AddDynamic(Relay, &ULWSliderRelay::HandleValueChanged);
	Relays.Add(Relay);

	if (UVerticalBoxSlot* PanelSlot = Cast<UVerticalBoxSlot>(Parent->AddChild(Row)))
	{
		PanelSlot->SetPadding(FMargin(0.f, 8.f));
	}
	Focusables.Add({ Slider, nullptr, Text });
}

void ULWMenuWidget::SetButtonLabel(UButton* Button, const FString& Label)
{
	if (Button)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Button->GetChildAt(0)))
		{
			Text->SetText(FText::FromString(Label));
		}
	}
}

UVerticalBox* ULWMenuWidget::AddColumn(UCanvasPanel* Root, const FVector2D& Anchor, const FVector2D& Alignment, float Width, const FVector2D& Offset)
{
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	Box->SetWidthOverride(Width);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Box->AddChild(Column);

	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Box);
	PanelSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
	PanelSlot->SetAlignment(Alignment);
	PanelSlot->SetPosition(Offset);
	PanelSlot->SetAutoSize(true);
	return Column;
}

void ULWMenuWidget::AddVeil(UCanvasPanel* Root, float Opacity)
{
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>();
	Veil->SetBrushColor(FLinearColor(0.f, 0.f, 0.02f, Opacity));
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Veil);
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	PanelSlot->SetOffsets(FMargin(0.f));
}

void ULWMenuWidget::FocusButton(int32 Index)
{
	if (Focusables.IsValidIndex(Index))
	{
		if (UWidget* Widget = Focusables[Index].Widget.Get())
		{
			Widget->SetKeyboardFocus();
			if (APlayerController* Controller = GetOwningPlayer())
			{
				Widget->SetUserFocus(Controller);
			}
		}
	}
}

void ULWMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// One highlight at a time: hovering with the mouse moves focus, so mouse and gamepad never disagree.
	for (const FFocusable& Item : Focusables)
	{
		UWidget* Widget = Item.Widget.Get();
		if (!Widget)
		{
			continue;
		}
		if (Widget->IsHovered() && !Widget->HasKeyboardFocus())
		{
			Widget->SetKeyboardFocus();
		}

		const bool bActive = Widget->HasKeyboardFocus() || Widget->HasAnyUserFocus();
		if (UButton* Button = Item.Button.Get())
		{
			Button->SetBackgroundColor(bActive ? ActiveFill : IdleFill);
		}
		if (UTextBlock* Label = Item.Label.Get())
		{
			Label->SetColorAndOpacity(FSlateColor(bActive ? Warm : Muted));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Intro

namespace
{
	// Seconds: the engine credit fades in and out, then the title does, then the menu takes over.
	constexpr float EngineIn = 0.4f, EngineHold = 2.2f, EngineOut = 2.9f;
	constexpr float TitleIn = 3.3f, TitleHold = 5.3f, TitleOut = 6.1f;

	float FadeWindow(float Time, float In, float Hold, float Out, float FadeTime = 0.6f)
	{
		if (Time < In || Time > Out) { return 0.f; }
		if (Time < In + FadeTime) { return (Time - In) / FadeTime; }
		if (Time > Hold) { return FMath::Clamp((Out - Time) / (Out - Hold), 0.f, 1.f); }
		return 1.f;
	}
}

void ULWIntroScreen::Build(UCanvasPanel* Root)
{
	SetIsFocusable(true);

	UBorder* Black = WidgetTree->ConstructWidget<UBorder>();
	Black->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 1.f));
	UCanvasPanelSlot* BlackSlot = Root->AddChildToCanvas(Black);
	BlackSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BlackSlot->SetOffsets(FMargin(0.f));
	Backdrop = Black;

	UVerticalBox* Centre = AddColumn(Root, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), 1400.f);

	// The official Unreal Engine emblem, as shipped with the engine (Engine/Content/Slate is staged with
	// packaged games), shown unaltered: original colours and proportions, with clear space around it.
	UImage* Logo = WidgetTree->ConstructWidget<UImage>();
	Logo->SetBrush(FSlateVectorImageBrush(FPaths::EngineContentDir() / TEXT("Slate/Starship/Common/UELogo.svg"), FVector2f(180.f, 180.f)));
	Logo->SetRenderOpacity(0.f);
	if (UVerticalBoxSlot* LogoSlot = Centre->AddChildToVerticalBox(Logo))
	{
		LogoSlot->SetHorizontalAlignment(HAlign_Center);
		LogoSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 36.f));
	}
	EngineLogo = Logo;

	EngineLine = MakeText(TEXT("POWERED BY"), 22, Muted, 400);
	EngineName = MakeText(TEXT("UNREAL ENGINE 5"), 54, FLinearColor::White, 250);
	Title = MakeText(TEXT("LIGHTWALKER"), 110, Glow, 300);
	for (UTextBlock* Text : { EngineLine.Get(), EngineName.Get(), Title.Get() })
	{
		Text->SetRenderOpacity(0.f);
		if (UVerticalBoxSlot* PanelSlot = Centre->AddChildToVerticalBox(Text))
		{
			PanelSlot->SetHorizontalAlignment(HAlign_Center);
			PanelSlot->SetPadding(FMargin(0.f, 4.f));
		}
	}
}

void ULWIntroScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	Time += InDeltaTime;
	const float Engine = FadeWindow(Time, EngineIn, EngineHold, EngineOut);
	EngineLogo->SetRenderOpacity(Engine);
	EngineLine->SetRenderOpacity(Engine);
	EngineName->SetRenderOpacity(Engine);
	EngineLogo->SetVisibility(Time < EngineOut ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	// The title shows over the arena: the black backdrop lifts as it fades in.
	Title->SetRenderOpacity(FadeWindow(Time, TitleIn, TitleHold, TitleOut, 1.f));
	Title->SetVisibility(Time >= TitleIn ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	EngineLine->SetVisibility(Time < EngineOut ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	EngineName->SetVisibility(Time < EngineOut ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Backdrop->SetRenderOpacity(1.f - FMath::Clamp((Time - TitleIn) / 2.f, 0.f, 1.f));

	if (Time >= TitleOut)
	{
		Skip();
	}
}

void ULWIntroScreen::Skip()
{
	if (!bFinished)
	{
		bFinished = true;
		if (OnFinished)
		{
			OnFinished();
		}
	}
}

FReply ULWIntroScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	Skip();
	return FReply::Handled();
}

FReply ULWIntroScreen::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Skip();
	return FReply::Handled();
}

// ---------------------------------------------------------------------------------------------------------------
// Main menu

void ULWMainMenu::Build(UCanvasPanel* Root)
{
	UVerticalBox* Header = AddColumn(Root, FVector2D(0.5f, 0.f), FVector2D(0.5f, 0.f), 1400.f, FVector2D(0.f, 70.f));
	Header->AddChildToVerticalBox(MakeText(TEXT("LIGHTWALKER"), 96, Glow, 300));
	Header->AddChildToVerticalBox(MakeText(TEXT("CARRY THE LIGHT"), 20, Muted, 600));

	UVerticalBox* Column = AddColumn(Root, FVector2D(0.07f, 0.55f), FVector2D(0.f, 0.5f), 420.f);
	AddButton(Column, TEXT("PLAY"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->StartGameFromMenu(); } });
	HeroButton = AddButton(Column, TEXT("HERO"), [this]()
	{
		if (ALightWalkerPlayerController* Controller = GetLWController())
		{
			Controller->CycleHero();
			RefreshHeroLabel();
		}
	});
	AddButton(Column, TEXT("SETTINGS"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->OpenSettings(); } });
	AddButton(Column, TEXT("QUIT"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->QuitGame(); } });
	RefreshHeroLabel();

	UVerticalBox* Footer = AddColumn(Root, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f), 1400.f, FVector2D(0.f, -36.f));
	Footer->AddChildToVerticalBox(MakeText(TEXT("ENTER / A  SELECT       ESC / B  BACK"), 16, Muted, 200));
}

void ULWMainMenu::RefreshHeroLabel()
{
	const ALightWalkerPlayerController* Controller = GetLWController();
	SetButtonLabel(HeroButton, FString::Printf(TEXT("HERO: %s"), Controller ? *Controller->GetHeroName() : TEXT("LIGHTWALKER")));
}

// ---------------------------------------------------------------------------------------------------------------
// Pause

void ULWPauseMenu::Build(UCanvasPanel* Root)
{
	AddVeil(Root, 0.6f);

	UVerticalBox* Column = AddColumn(Root, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), 440.f);
	if (UVerticalBoxSlot* PanelSlot = Column->AddChildToVerticalBox(MakeText(TEXT("PAUSED"), 56, Glow, 300)))
	{
		PanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
	}
	AddButton(Column, TEXT("RESUME"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->SetMenuPaused(false); } });
	AddButton(Column, TEXT("RESTART"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->RestartRun(); } });
	AddButton(Column, TEXT("SETTINGS"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->OpenSettings(); } });
	AddButton(Column, TEXT("QUIT TO MENU"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->QuitToMenu(); } });
}

// ---------------------------------------------------------------------------------------------------------------
// Settings

void ULWSettingsMenu::Build(UCanvasPanel* Root)
{
	AddVeil(Root, 0.75f);

	ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
	UVerticalBox* Column = AddColumn(Root, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), 680.f);
	if (UVerticalBoxSlot* PanelSlot = Column->AddChildToVerticalBox(MakeText(TEXT("SETTINGS"), 48, Glow, 300)))
	{
		PanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	}

	QualityButton = AddButton(Column, TEXT("GRAPHICS"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			const int32 Level = S->GetOverallScalabilityLevel();
			S->SetOverallScalabilityLevel(Level < 0 || Level >= 3 ? 0 : Level + 1);
			S->ApplyNonResolutionSettings();
			Refresh();
		}
	});

	WindowButton = AddButton(Column, TEXT("DISPLAY"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			const EWindowMode::Type Mode = S->GetFullscreenMode();
			S->SetFullscreenMode(Mode == EWindowMode::Windowed ? EWindowMode::WindowedFullscreen
				: Mode == EWindowMode::WindowedFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed);
			S->ApplyResolutionSettings(false);
			Refresh();
		}
	});

	ResolutionButton = AddButton(Column, TEXT("RESOLUTION"), [this]()
	{
		ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get();
		TArray<FIntPoint> Resolutions;
		if (!S || !UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions) || Resolutions.Num() == 0)
		{
			return;
		}
		const int32 Current = Resolutions.IndexOfByKey(S->GetScreenResolution());
		S->SetScreenResolution(Resolutions[(Current + 1) % Resolutions.Num()]);
		S->ApplyResolutionSettings(false);
		Refresh();
	});

	VSyncButton = AddButton(Column, TEXT("V-SYNC"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			S->SetVSyncEnabled(!S->IsVSyncEnabled());
			S->ApplyNonResolutionSettings();
			Refresh();
		}
	});

	AddSlider(Column, TEXT("MASTER VOLUME"), Settings ? Settings->MasterVolume : 1.f, [](float Value) { if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get()) { S->MasterVolume = Value; } });
	AddSlider(Column, TEXT("MUSIC"), Settings ? Settings->MusicVolume : 0.8f, [](float Value) { if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get()) { S->MusicVolume = Value; } });
	AddSlider(Column, TEXT("EFFECTS"), Settings ? Settings->EffectsVolume : 1.f, [](float Value) { if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get()) { S->EffectsVolume = Value; } });

	VibrationButton = AddButton(Column, TEXT("VIBRATION"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			S->bVibration = !S->bVibration;
			Refresh();
		}
	});

	AddButton(Column, TEXT("BACK"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			S->SaveSettings();
		}
		if (OnBack)
		{
			OnBack();
		}
	});

	Refresh();
}

void ULWSettingsMenu::Refresh()
{
	const ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get();
	if (!S)
	{
		return;
	}
	const FIntPoint Resolution = S->GetScreenResolution();
	SetButtonLabel(QualityButton, FString::Printf(TEXT("GRAPHICS: %s"), *QualityName(S->GetOverallScalabilityLevel())));
	SetButtonLabel(WindowButton, FString::Printf(TEXT("DISPLAY: %s"), *WindowName(S->GetFullscreenMode())));
	SetButtonLabel(ResolutionButton, FString::Printf(TEXT("RESOLUTION: %d x %d"), Resolution.X, Resolution.Y));
	SetButtonLabel(VSyncButton, FString::Printf(TEXT("V-SYNC: %s"), S->IsVSyncEnabled() ? TEXT("ON") : TEXT("OFF")));
	SetButtonLabel(VibrationButton, FString::Printf(TEXT("VIBRATION: %s"), S->bVibration ? TEXT("ON") : TEXT("OFF")));
}
