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
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/LWSound.h"
#include "Core/LightWalkerUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/LightWalkerPlayerController.h"
#include "UI/LWStyle.h"

namespace
{
	/** Buttons lean forward like italic type. */
	constexpr float ButtonShear = -12.f;

	/** Space under a panel title, so the first row clears the frame's top edge and its corner details. */
	constexpr float PanelTitleGap = 56.f;

	FLinearColor WithAlpha(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Alpha);
	}

	FSlateBrush NeonBox(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth, float Radius = 4.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return Brush;
	}

	/** True when the ChatGPT button art is imported (the buttons are then drawn from it and need no shear). */
	bool HasButtonArt()
	{
		return LWStyle::Art(TEXT("ButtonNormal")) && LWStyle::Art(TEXT("ButtonHover")) && LWStyle::Art(TEXT("ButtonPressed"));
	}

	FButtonStyle MakeButtonStyle(bool bActive)
	{
		if (HasButtonArt())
		{
			// 9-slice so the slanted ends and corner tab keep their shape at any button width.
			// Cropped tight to the frame; the slanted ends keep their width, the middle stretches.
			const FVector2D Size(420.f, 81.f);
			const FMargin Slice(0.14f, 0.f, 0.14f, 0.f);
			const FSlateBrush Idle = LWStyle::ArtBrush(LWStyle::Art(bActive ? TEXT("ButtonHover") : TEXT("ButtonNormal")), Size, Slice);
			const FSlateBrush Pressed = LWStyle::ArtBrush(LWStyle::Art(TEXT("ButtonPressed")), Size, Slice);
			FButtonStyle Style;
			Style.SetNormal(Idle).SetHovered(Idle).SetPressed(Pressed).SetDisabled(Idle);
			Style.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
			return Style;
		}

		// Built-in look. Idle: dark glass with a thin cyan edge. Active: a hot pink edge with a pink glow inside.
		const FSlateBrush Brush = bActive
			? NeonBox(WithAlpha(LWStyle::Pink, 0.24f), LWStyle::Pink, 2.5f)
			: NeonBox(WithAlpha(LWStyle::Deep, 0.62f), WithAlpha(LWStyle::Cyan, 0.45f), 1.5f);
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

void ULWMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	AppearTime = 0.f;
}

ALightWalkerPlayerController* ULWMenuWidget::GetLWController() const
{
	return Cast<ALightWalkerPlayerController>(GetOwningPlayer());
}

UTextBlock* ULWMenuWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color, int32 LetterSpacing)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
	Block->SetFont(LWStyle::Display(Size, LetterSpacing));
	Block->SetText(FText::FromString(Text));
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetShadowOffset(FVector2D(2.f, 2.f));
	Block->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Block->SetJustification(ETextJustify::Center);
	return Block;
}

UTextBlock* ULWMenuWidget::MakeBody(const FString& Text, int32 Size, const FLinearColor& Color, int32 LetterSpacing)
{
	UTextBlock* Block = MakeText(Text, Size, Color, LetterSpacing);
	Block->SetFont(LWStyle::Body(Size, true, LetterSpacing));
	return Block;
}

UTextBlock* ULWMenuWidget::MakeTitle(const FString& Text, int32 Size)
{
	// Hot pink lettering with an offset cyan "shadow": the doubled-neon look of a sign seen through haze.
	UTextBlock* Block = MakeText(Text, Size, LWStyle::Pink, 260);
	Block->SetShadowOffset(FVector2D(Size * 0.045f, Size * 0.045f));
	Block->SetShadowColorAndOpacity(WithAlpha(LWStyle::Cyan, 0.75f));
	Block->SetRenderShear(FVector2D(ButtonShear * 0.6f, 0.f));
	return Block;
}

UWidget* ULWMenuWidget::MakeLogo(float Width, int32 FallbackTextSize)
{
	if (UTexture2D* Logo = LWStyle::Art(TEXT("Logo")))
	{
		// Fixed size, keeping the art's proportions.
		// The size the art was made at: the GPU copy may be padded to a power of two.
		const FIntPoint ArtSize = Logo->GetImportedSize();
		const float Height = Width * ArtSize.Y / FMath::Max(1, ArtSize.X);
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrush(LWStyle::ArtBrush(Logo, FVector2D(Width, Height)));
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(Width);
		Box->SetHeightOverride(Height);
		Box->AddChild(Image);
		return Box;
	}
	return MakeTitle(TEXT("LIGHTWALKER"), FallbackTextSize);
}

UButton* ULWMenuWidget::AddButton(UPanelWidget* Parent, const FString& Label, TFunction<void()> OnClick)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetStyle(MakeButtonStyle(false));
	const bool bArt = HasButtonArt();
	if (!bArt)
	{
		Button->SetRenderShear(FVector2D(ButtonShear, 0.f));
	}

	// Content: the label.
	UOverlay* Content = WidgetTree->ConstructWidget<UOverlay>();
	Content->SetClipping(EWidgetClipping::ClipToBounds);
	UTextBlock* Text = MakeText(Label, 22, LWStyle::Muted, 160);
	if (bArt)
	{
		Text->SetRenderShear(FVector2D(ButtonShear * 0.7f, 0.f));  // italic, to match the slanted art
	}
	if (UOverlaySlot* TextSlot = Content->AddChildToOverlay(Text))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(bArt ? FMargin(48.f, 20.f) : FMargin(28.f, 13.f));
	}
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->AddChild(Content)))
	{
		ContentSlot->SetPadding(FMargin(0.f));
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ULWClickRelay* Relay = NewObject<ULWClickRelay>(this);
	Relay->Callback = [this, Click = MoveTemp(OnClick)]()
	{
		ULWSoundSubsystem::Play(this, ELWSound::UISelect);
		Click();
	};
	Button->OnClicked.AddDynamic(Relay, &ULWClickRelay::HandleClicked);
	Relays.Add(Relay);

	if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Parent->AddChild(Button)))
	{
		VerticalSlot->SetPadding(FMargin(0.f, 7.f));
		VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	FFocusable& Item = Focusables.AddDefaulted_GetRef();
	Item.Widget = Button;
	Item.Button = Button;
	Item.Label = Text;
	return Button;
}

void ULWMenuWidget::AddSlider(UPanelWidget* Parent, const FString& Label, float Value, TFunction<void(float)> OnChange)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

	UTextBlock* Text = MakeBody(Label, 22, LWStyle::Muted, 180);
	Text->SetJustification(ETextJustify::Left);
	if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Text))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetPadding(FMargin(10.f, 0.f));
	}

	USlider* Slider = WidgetTree->ConstructWidget<USlider>();
	Slider->SetValue(Value);
	Slider->SetStepSize(0.05f);
	Slider->SetSliderBarColor(WithAlpha(LWStyle::Magenta * 0.5f, 1.f));
	Slider->SetSliderHandleColor(LWStyle::Pink);
	USizeBox* SliderBox = WidgetTree->ConstructWidget<USizeBox>();
	SliderBox->SetWidthOverride(200.f);
	SliderBox->AddChild(Slider);
	if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(SliderBox))
	{
		SliderSlot->SetVerticalAlignment(VAlign_Center);
		SliderSlot->SetPadding(FMargin(10.f, 0.f));
	}

	ULWSliderRelay* Relay = NewObject<ULWSliderRelay>(this);
	Relay->Callback = MoveTemp(OnChange);
	Slider->OnValueChanged.AddDynamic(Relay, &ULWSliderRelay::HandleValueChanged);
	Relays.Add(Relay);

	if (UVerticalBoxSlot* PanelSlot = Cast<UVerticalBoxSlot>(Parent->AddChild(Row)))
	{
		PanelSlot->SetPadding(FMargin(0.f, 8.f));
	}
	FFocusable& Item = Focusables.AddDefaulted_GetRef();
	Item.Widget = Slider;
	Item.Label = Text;
}

void ULWMenuWidget::SetButtonLabel(UButton* Button, const FString& Label)
{
	for (const FFocusable& Item : Focusables)
	{
		if (Item.Button.Get() == Button && Item.Label.IsValid())
		{
			Item.Label->SetText(FText::FromString(Label));
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
	SlideIns.Add(Box);
	return Column;
}

UVerticalBox* ULWMenuWidget::AddPanel(UCanvasPanel* Root, float Width)
{
	// A dark glass panel with a neon edge, centred, holding a column: the ChatGPT frame art if imported
	// (9-slice, keeping its corner details), otherwise the built-in pink outline.
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	if (UTexture2D* Frame = LWStyle::Art(TEXT("Panel")))
	{
		Panel->SetBrush(LWStyle::ArtBrush(Frame, FVector2D(768.f, 512.f), FMargin(0.2f, 0.25f, 0.2f, 0.25f)));
		Panel->SetPadding(FMargin(90.f, 70.f, 90.f, 220.f));  // clear of the frame's lower corner details
	}
	else
	{
		Panel->SetBrush(NeonBox(WithAlpha(LWStyle::Deep, 0.82f), WithAlpha(LWStyle::Pink, 0.8f), 2.f, 10.f));
		Panel->SetPadding(FMargin(44.f, 36.f));
	}

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	Box->SetWidthOverride(Width);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Box->AddChild(Column);
	Panel->AddChild(Box);

	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);
	SlideIns.Add(Panel);
	return Column;
}

void ULWMenuWidget::AddVeil(UCanvasPanel* Root, float Opacity)
{
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>();
	Veil->SetBrushColor(FLinearColor(0.01f, 0.f, 0.04f, Opacity));
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

	// Columns and panels slide in from the left and fade up when the menu opens.
	AppearTime += InDeltaTime;
	const float Appear = FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(AppearTime / 0.35f, 0.f, 1.f), 3.f);
	for (const TWeakObjectPtr<UWidget>& Widget : SlideIns)
	{
		if (UWidget* Panel = Widget.Get())
		{
			Panel->SetRenderTranslation(FVector2D((1.f - Appear) * -50.f, 0.f));
			Panel->SetRenderOpacity(Appear);
		}
	}

	// One highlight at a time: hovering with the mouse moves focus, so mouse and gamepad never disagree.
	UWidget* Focused = nullptr;
	const float Pulse = 0.85f + 0.15f * FMath::Sin(AppearTime * 5.f);
	for (FFocusable& Item : Focusables)
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
		if (bActive)
		{
			Focused = Widget;
		}
		if (UButton* Button = Item.Button.Get())
		{
			if (bActive != Item.bWasActive)
			{
				Button->SetStyle(MakeButtonStyle(bActive));
			}
			Button->SetBackgroundColor(bActive ? FLinearColor(Pulse, Pulse, Pulse, 1.f) : FLinearColor::White);
		}
		if (UTextBlock* Label = Item.Label.Get())
		{
			Label->SetColorAndOpacity(FSlateColor(bActive ? LWStyle::Text : LWStyle::Muted));
		}
		Item.bWasActive = bActive;
	}

	if (Focused && LastFocused.IsValid() && Focused != LastFocused.Get())
	{
		ULWSoundSubsystem::Play(this, ELWSound::UIMove);
	}
	if (Focused)
	{
		LastFocused = Focused;
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Intro

namespace
{
	// Seconds. The engine credit is Epic's animated logo, played as a startup movie before this (DefaultGame.ini),
	// so the intro is just the title rising out of black, then the menu takes over.
	constexpr float TitleIn = 0.3f, TitleHold = 2.6f, TitleOut = 3.4f;

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

	// The neon sunset art behind the logo if it's imported, plain black otherwise.
	UWidget* Back = nullptr;
	if (UTexture2D* Sunset = LWStyle::Art(TEXT("MenuBackground")))
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrush(LWStyle::ArtBrush(Sunset, FVector2D(1920.f, 1080.f)));
		Back = Image;
		bArtBackdrop = true;
	}
	else
	{
		UBorder* Black = WidgetTree->ConstructWidget<UBorder>();
		Black->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 1.f));
		Back = Black;
	}
	UCanvasPanelSlot* BackSlot = Root->AddChildToCanvas(Back);
	BackSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackSlot->SetOffsets(FMargin(0.f));
	Backdrop = Back;

	UVerticalBox* Centre = AddColumn(Root, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), 1400.f);
	Title = MakeLogo(1100.f, 110);
	Title->SetRenderOpacity(0.f);
	if (UVerticalBoxSlot* PanelSlot = Centre->AddChildToVerticalBox(Title))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void ULWIntroScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	Time += InDeltaTime;

	Title->SetRenderOpacity(FadeWindow(Time, TitleIn, TitleHold, TitleOut, 1.f));
	if (bArtBackdrop)
	{
		// The logo glows over the sunset, then both fade away to reveal Lightwalker in the arena.
		Backdrop->SetRenderOpacity(FMath::Clamp((TitleOut - Time) / (TitleOut - TitleHold), 0.f, 1.f));
	}
	else
	{
		// The title shows over the arena: the black backdrop lifts as it fades in.
		Backdrop->SetRenderOpacity(1.f - FMath::Clamp((Time - TitleIn) / 2.f, 0.f, 1.f));
	}

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
	// The logo, top centre.
	UWidget* Logo = MakeLogo(640.f, 96);
	UCanvasPanelSlot* LogoSlot = Root->AddChildToCanvas(Logo);
	LogoSlot->SetAnchors(FAnchors(0.5f, 0.f));
	LogoSlot->SetAlignment(FVector2D(0.5f, 0.f));
	LogoSlot->SetPosition(FVector2D(0.f, 24.f));
	LogoSlot->SetAutoSize(true);
	AddSlideIn(Logo);

	// The chosen hero's concept art on the right, standing in the lantern light on the arena floor (the 3D model,
	// built to be seen from far above, is hidden on the menu).
	// The art and the lantern's halo share a stage, so the halo can sit over the lantern of either hero.
	UCanvasPanel* HeroStage = WidgetTree->ConstructWidget<UCanvasPanel>();
	HeroStage->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* StageSlot = Root->AddChildToCanvas(HeroStage);
	StageSlot->SetAnchors(FAnchors(0.79f, 1.f));
	StageSlot->SetAlignment(FVector2D(0.5f, 1.f));
	StageSlot->SetPosition(FVector2D(0.f, -40.f));
	StageSlot->SetSize(FVector2D(HeroArtSize, HeroArtSize));
	AddSlideIn(HeroStage);

	HeroImage = WidgetTree->ConstructWidget<UImage>();
	UCanvasPanelSlot* HeroSlot = HeroStage->AddChildToCanvas(HeroImage);
	HeroSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	HeroSlot->SetOffsets(FMargin(0.f));

	// Cutting the art out of its background took the lantern's halo with it; this puts a flickering one back.
	if (UTexture2D* Glow = LWStyle::Art(TEXT("LanternGlow")))
	{
		LanternGlow = WidgetTree->ConstructWidget<UImage>();
		LanternGlow->SetBrush(LWStyle::ArtBrush(Glow, FVector2D(LanternGlowSize, LanternGlowSize)));
		UCanvasPanelSlot* GlowSlot = HeroStage->AddChildToCanvas(LanternGlow);
		GlowSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		GlowSlot->SetSize(FVector2D(LanternGlowSize, LanternGlowSize));
	}

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
	Footer->AddChildToVerticalBox(MakeBody(TEXT("ENTER / A  SELECT       ESC / B  BACK"), 18, LWStyle::Muted, 300));
}

void ULWMainMenu::RefreshHeroLabel()
{
	const ALightWalkerPlayerController* Controller = GetLWController();
	SetButtonLabel(HeroButton, FString::Printf(TEXT("HERO: %s"), Controller ? *Controller->GetHeroName() : TEXT("LIGHTWALKER")));

	if (HeroImage)
	{
		const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
		UTexture2D* Art = (Settings && Settings->HeroIndex == 1) ? LWStyle::Art(TEXT("HeroFemale")) : nullptr;
		Art = Art ? Art : LWStyle::Art(TEXT("HeroMale"));
		HeroImage->SetVisibility(Art ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Art)
		{
			HeroImage->SetBrush(LWStyle::ArtBrush(Art, FVector2D(HeroArtSize, HeroArtSize)));
		}

		if (LanternGlow)
		{
			// Where each concept's lantern glass sits, as a fraction of the art.
			const bool bFemale = Art && Art == LWStyle::Art(TEXT("HeroFemale"));
			const FVector2D Lantern = bFemale ? FVector2D(0.285f, 0.121f) : FVector2D(0.262f, 0.161f);
			LanternGlow->SetVisibility(Art ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* GlowSlot = Cast<UCanvasPanelSlot>(LanternGlow->Slot))
			{
				GlowSlot->SetPosition(Lantern * HeroArtSize);
			}
		}
	}
}

void ULWMainMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// A slow breath with a faint flame flicker on top.
	GlowTime += InDeltaTime;
	if (LanternGlow)
	{
		LanternGlow->SetRenderOpacity(0.86f + 0.09f * FMath::Sin(GlowTime * 2.1f) + 0.05f * FMath::Sin(GlowTime * 7.3f + 1.f));
		LanternGlow->SetRenderScale(FVector2D(1.f + 0.03f * FMath::Sin(GlowTime * 2.1f)));
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Pause

void ULWPauseMenu::Build(UCanvasPanel* Root)
{
	AddVeil(Root, 0.6f);

	UVerticalBox* Column = AddPanel(Root, 440.f);
	if (UVerticalBoxSlot* PanelSlot = Column->AddChildToVerticalBox(MakeTitle(TEXT("PAUSED"), 56)))
	{
		PanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, PanelTitleGap));
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
	UVerticalBox* Column = AddPanel(Root, 1120.f);
	if (UVerticalBoxSlot* PanelSlot = Column->AddChildToVerticalBox(MakeTitle(TEXT("SETTINGS"), 48)))
	{
		PanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, PanelTitleGap));
	}

	// Two columns so the page fits a 16:9 screen: display on the left, sound and feel on the right.
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(Columns);
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>();
	for (UVerticalBox* Side : { Left, Right })
	{
		if (UHorizontalBoxSlot* SideSlot = Columns->AddChildToHorizontalBox(Side))
		{
			SideSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SideSlot->SetPadding(FMargin(Side == Left ? 0.f : 20.f, 0.f, Side == Left ? 20.f : 0.f, 0.f));
		}
	}

	QualityButton = AddButton(Left, TEXT("GRAPHICS"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			const int32 Level = S->GetOverallScalabilityLevel();
			S->SetOverallScalabilityLevel(Level < 0 || Level >= 3 ? 0 : Level + 1);
			S->ApplyNonResolutionSettings();
			Refresh();
		}
	});

	WindowButton = AddButton(Left, TEXT("DISPLAY"), [this]()
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

	ResolutionButton = AddButton(Left, TEXT("RESOLUTION"), [this]()
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

	FrameRateButton = AddButton(Left, TEXT("FRAME RATE"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			const TArray<float> Options = ULightWalkerUserSettings::GetFrameRateOptions();
			const int32 Current = Options.IndexOfByPredicate([S](float Rate) { return FMath::IsNearlyEqual(Rate, S->GetFrameRateLimit()); });
			S->SetFrameRateLimit(Options[(Current + 1) % Options.Num()]);
			S->ApplyNonResolutionSettings();
			Refresh();
		}
	});

	VSyncButton = AddButton(Left, TEXT("V-SYNC"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			S->SetVSyncEnabled(!S->IsVSyncEnabled());
			S->ApplyNonResolutionSettings();
			Refresh();
		}
	});

	AddSlider(Right, TEXT("MASTER VOLUME"), Settings ? Settings->MasterVolume : 1.f, [](float Value) { if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get()) { S->MasterVolume = Value; } });
	AddSlider(Right, TEXT("MUSIC"), Settings ? Settings->MusicVolume : 0.8f, [](float Value) { if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get()) { S->MusicVolume = Value; } });
	AddSlider(Right, TEXT("EFFECTS"), Settings ? Settings->EffectsVolume : 1.f, [](float Value) { if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get()) { S->EffectsVolume = Value; } });

	VibrationButton = AddButton(Right, TEXT("VIBRATION"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			S->bVibration = !S->bVibration;
			Refresh();
		}
	});

	ShakeButton = AddButton(Right, TEXT("SCREEN SHAKE"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			S->bScreenShake = !S->bScreenShake;
			Refresh();
		}
	});

	AddButton(Right, TEXT("CONTROLS"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->OpenControls(); } });

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
	const float FrameRate = S->GetFrameRateLimit();
	SetButtonLabel(FrameRateButton, FrameRate > 0.f ? FString::Printf(TEXT("FRAME RATE: %d FPS"), FMath::RoundToInt(FrameRate)) : FString(TEXT("FRAME RATE: UNLIMITED")));
	SetButtonLabel(VSyncButton, FString::Printf(TEXT("V-SYNC: %s"), S->IsVSyncEnabled() ? TEXT("ON") : TEXT("OFF")));
	SetButtonLabel(VibrationButton, FString::Printf(TEXT("VIBRATION: %s"), S->bVibration ? TEXT("ON") : TEXT("OFF")));
	SetButtonLabel(ShakeButton, FString::Printf(TEXT("SCREEN SHAKE: %s"), S->bScreenShake ? TEXT("ON") : TEXT("OFF")));
}

// ---------------------------------------------------------------------------------------------------------------
// Controls

namespace
{
	/** Gives up waiting for a key after this long (a gamepad has no Esc to cancel with). */
	constexpr float ListenTimeout = 6.f;
}

void ULWControlsMenu::Build(UCanvasPanel* Root)
{
	AddVeil(Root, 0.75f);

	UVerticalBox* Column = AddPanel(Root, 680.f);
	if (UVerticalBoxSlot* PanelSlot = Column->AddChildToVerticalBox(MakeTitle(TEXT("CONTROLS"), 48)))
	{
		PanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, PanelTitleGap));
	}

	for (int32 Index = 0; Index < static_cast<int32>(ELWBinding::Count); ++Index)
	{
		BindingButtons.Add(AddButton(Column, ULightWalkerUserSettings::GetBindingLabel(static_cast<ELWBinding>(Index)), [this, Index]()
		{
			Listening = Index;
			ListenAge = 0.f;
			Refresh();
		}));
	}

	AddButton(Column, TEXT("RESET TO DEFAULTS"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			S->ResetBindings();
			S->SaveSettings();
		}
		if (ALightWalkerPlayerController* Controller = GetLWController())
		{
			Controller->ApplyBindings();
		}
		StopListening();
	});
	AddButton(Column, TEXT("BACK"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->CloseControls(); } });

	// What stays fixed, along the bottom of the screen like the main menu's footer.
	UVerticalBox* Footer = AddColumn(Root, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f), 1400.f, FVector2D(0.f, -24.f));
	Footer->AddChildToVerticalBox(MakeBody(TEXT("ARROW KEYS AND LEFT STICK ALSO MOVE       MOUSE OR RIGHT STICK AIMS       ESC PAUSES"), 18, LWStyle::Muted, 200));

	Refresh();
}

void ULWControlsMenu::Refresh()
{
	const ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get();
	for (int32 Index = 0; Index < BindingButtons.Num(); ++Index)
	{
		const ELWBinding Binding = static_cast<ELWBinding>(Index);
		FString Label = ULightWalkerUserSettings::GetBindingLabel(Binding) + TEXT(":  ");
		if (Index == Listening)
		{
			Label += TEXT("PRESS A KEY OR BUTTON");
		}
		else
		{
			const FKey Key = S ? S->GetBinding(Binding, false) : ULightWalkerUserSettings::GetDefaultBinding(Binding, false);
			const FKey Button = S ? S->GetBinding(Binding, true) : ULightWalkerUserSettings::GetDefaultBinding(Binding, true);
			Label += ULightWalkerUserSettings::GetKeyLabel(Key) + TEXT("  /  ") + ULightWalkerUserSettings::GetKeyLabel(Button);
		}
		SetButtonLabel(BindingButtons[Index], Label);
	}
}

void ULWControlsMenu::StopListening()
{
	Listening = -1;
	Refresh();
}

void ULWControlsMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Listening >= 0)
	{
		ListenAge += InDeltaTime;
		if (ListenAge > ListenTimeout)
		{
			StopListening();
		}
	}
}

FReply ULWControlsMenu::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (Listening < 0)
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}

	// While listening every key is ours: none of them navigate, pause or go back.
	const FKey Key = InKeyEvent.GetKey();
	if (InKeyEvent.IsRepeat() || ListenAge < 0.15f)
	{
		return FReply::Handled();
	}
	if (Key == EKeys::Escape)
	{
		ULWSoundSubsystem::Play(this, ELWSound::UIMove);
		StopListening();
		return FReply::Handled();
	}

	ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get();
	if (S && S->SetBinding(static_cast<ELWBinding>(Listening), Key))
	{
		S->SaveSettings();
		if (ALightWalkerPlayerController* Controller = GetLWController())
		{
			Controller->ApplyBindings();
		}
		ULWSoundSubsystem::Play(this, ELWSound::UISelect);
		StopListening();
	}
	// Keys that can't be bound (sticks, arrows) are ignored; keep listening.
	return FReply::Handled();
}

FReply ULWControlsMenu::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Listening >= 0)
	{
		// A click while listening cancels rather than pressing another button.
		StopListening();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void ULWControlsMenu::NativeDestruct()
{
	Listening = -1;
	Super::NativeDestruct();
}
