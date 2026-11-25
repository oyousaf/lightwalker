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
#include "Components/ScaleBox.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/LWNeon.h"
#include "Core/LWScoreboard.h"
#include "Core/LWSound.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
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

	using LWStyle::WithAlpha;

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
			// Cropped tight to the frame, and drawn whole: buttons are close to the art's own shape. (9-slicing needs ends
			// no wider than the button in the art's own pixels, and at 1450 px across they overlapped in the middle.)
			const FVector2D Size(420.f, 81.f);
			const FSlateBrush Idle = LWStyle::ArtBrush(LWStyle::Art(bActive ? TEXT("ButtonHover") : TEXT("ButtonNormal")), Size);
			const FSlateBrush Pressed = LWStyle::ArtBrush(LWStyle::Art(TEXT("ButtonPressed")), Size);
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

	FIntPoint ViewSize(const UWorld* World)
	{
		FVector2D Size = FVector2D::ZeroVector;
		if (const UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr)
		{
			Viewport->GetViewportSize(Size);
		}
		const ULightWalkerUserSettings* Settings = ULightWalkerUserSettings::Get();
		if (Size.X <= 0.f && Settings)
		{
			return Settings->GetScreenResolution();  // no view yet
		}
		return FIntPoint(FMath::RoundToInt(Size.X), FMath::RoundToInt(Size.Y));
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
		// Backdrops, veils and motes cover the whole screen; everything else lives in a centred area no wider than
		// 16:9, so on ultrawide screens the buttons and panels stay in front of the player instead of at the edges.
		FullScreen = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = FullScreen;
		USizeBox* Safe = WidgetTree->ConstructWidget<USizeBox>();
		Safe->SetMaxAspectRatio(16.f / 9.f);
		UCanvasPanelSlot* SafeSlot = FullScreen->AddChildToCanvas(Safe);
		SafeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		SafeSlot->SetOffsets(FMargin(0.f));
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
		Safe->AddChild(Root);
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
	UTextBlock* Text = MakeText(Label, 22, LWStyle::Muted, 60);
	if (bArt)
	{
		Text->SetRenderShear(FVector2D(ButtonShear * 0.7f, 0.f));  // italic, to match the slanted art
	}
	// A long label (a 4K resolution) shrinks to stay inside the frame.
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFitX);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	Fit->AddChild(Text);
	if (UOverlaySlot* TextSlot = Content->AddChildToOverlay(Fit))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(bArt ? FMargin(70.f, 20.f) : FMargin(28.f, 13.f));
	}
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->AddChild(Content)))
	{
		ContentSlot->SetPadding(FMargin(0.f));
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ULWClickRelay* Relay = NewObject<ULWClickRelay>(this);
	Relay->Callback = [this, Button, Click = MoveTemp(OnClick)]()
	{
		if (IsButtonLocked(Button))
		{
			ULWSoundSubsystem::Play(this, ELWSound::WallHit);
			OnLockedPressed(Button);
			return;
		}
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

void ULWMenuWidget::SetButtonLocked(UButton* Button, bool bLocked)
{
	for (FFocusable& Item : Focusables)
	{
		UOverlay* Content = Item.Button.Get() == Button ? Cast<UOverlay>(Button->GetContent()) : nullptr;
		if (!Content)
		{
			continue;
		}
		Item.bLocked = bLocked;
		Content->SetRenderOpacity(bLocked ? 0.55f : 1.f);
		if (bLocked && !Item.Padlock.IsValid())
		{
			// A padlock: a hoop behind a solid body.
			UOverlay* Padlock = WidgetTree->ConstructWidget<UOverlay>();
			auto AddPart = [&](const FSlateBrush& Brush, float Width, float Height, EVerticalAlignment Align)
			{
				USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
				Box->SetWidthOverride(Width);
				Box->SetHeightOverride(Height);
				UImage* Part = WidgetTree->ConstructWidget<UImage>();
				Part->SetBrush(Brush);
				Box->AddChild(Part);
				if (UOverlaySlot* PartSlot = Padlock->AddChildToOverlay(Box))
				{
					PartSlot->SetHorizontalAlignment(HAlign_Center);
					PartSlot->SetVerticalAlignment(Align);
				}
			};
			AddPart(NeonBox(FLinearColor::Transparent, LWStyle::Text, 2.5f, 6.f), 14.f, 20.f, VAlign_Top);
			AddPart(NeonBox(LWStyle::Text, LWStyle::Text, 0.f, 3.f), 20.f, 15.f, VAlign_Bottom);
			USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>();
			Frame->SetWidthOverride(22.f);
			Frame->SetHeightOverride(26.f);
			Frame->AddChild(Padlock);
			if (UOverlaySlot* FrameSlot = Content->AddChildToOverlay(Frame))
			{
				FrameSlot->SetHorizontalAlignment(HAlign_Left);
				FrameSlot->SetVerticalAlignment(VAlign_Center);
				FrameSlot->SetPadding(FMargin(78.f, 0.f, 0.f, 4.f));
			}
			Item.Padlock = Frame;
		}
		if (UWidget* Padlock = Item.Padlock.Get())
		{
			Padlock->SetVisibility(bLocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

bool ULWMenuWidget::IsButtonLocked(const UButton* Button) const
{
	return Focusables.ContainsByPredicate([Button](const FFocusable& Item) { return Item.bLocked && Item.Button.Get() == Button; });
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

void ULWMenuWidget::AddFullScreen(UWidget* Widget)
{
	// Behind the centred content, in the order added.
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(FullScreen->InsertChildAt(FullScreenLayers++, Widget)))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		PanelSlot->SetOffsets(FMargin(0.f));
	}
}

UWidget* ULWMenuWidget::AddVeil(UCanvasPanel* Root, float Opacity)
{
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>();
	Veil->SetBrushColor(FLinearColor(0.01f, 0.f, 0.04f, Opacity));
	AddFullScreen(Veil);
	return Veil;
}

UWidget* ULWMenuWidget::AddCopyright(UCanvasPanel* Root)
{
	UTextBlock* Line = MakeBody(LWStyle::Copyright() + TEXT("     ") + LWStyle::Version(), 14, WithAlpha(LWStyle::Muted, 0.75f), 160);
	UCanvasPanelSlot* LineSlot = Root->AddChildToCanvas(Line);
	LineSlot->SetAnchors(FAnchors(0.5f, 1.f));
	LineSlot->SetAlignment(FVector2D(0.5f, 1.f));
	LineSlot->SetPosition(FVector2D(0.f, -22.f));
	LineSlot->SetAutoSize(true);
	return Line;
}

UTexture2D* ULWMenuWidget::PickBackdropArt(float Aspect)
{
	// The sunset drawn for the screen's shape: 16:9, plus 21:9 and 32:9 versions if they've been imported.
	struct FVersion { const TCHAR* Name; float Aspect; };
	static const FVersion Versions[] = {
		{ TEXT("MenuBackground"), 16.f / 9.f },
		{ TEXT("MenuBackground_21x9"), 21.f / 9.f },
		{ TEXT("MenuBackground_32x9"), 32.f / 9.f },
	};
	UTexture2D* Best = nullptr;
	float BestGap = TNumericLimits<float>::Max();
	for (const FVersion& Version : Versions)
	{
		UTexture2D* Art = LWStyle::Art(Version.Name);
		const float Gap = FMath::Abs(FMath::Loge(Aspect / Version.Aspect));
		if (Art && Gap < BestGap)
		{
			Best = Art;
			BestGap = Gap;
		}
	}
	return Best;
}

void ULWMenuWidget::SetBackdropArt(UTexture2D* Art)
{
	if (UImage* Image = BackdropImage.Get())
	{
		const FIntPoint Size = Art->GetImportedSize();
		Image->SetBrush(LWStyle::ArtBrush(Art, FVector2D(Size.X, Size.Y)));
		BackdropArt = Art;
	}
}

UWidget* ULWMenuWidget::AddBackdrop(UCanvasPanel* Root)
{
	UTexture2D* Sunset = PickBackdropArt(LWNeon::GetViewAspect(GetWorld()));
	if (!Sunset)
	{
		return nullptr;
	}
	// Scaled to cover the screen at any shape, cropping rather than stretching; NativeTick swaps in the version
	// drawn for the screen's shape if the window changes.
	UImage* Image = WidgetTree->ConstructWidget<UImage>();
	BackdropImage = Image;
	SetBackdropArt(Sunset);
	UScaleBox* Cover = WidgetTree->ConstructWidget<UScaleBox>();
	Cover->SetStretch(EStretch::ScaleToFill);
	Cover->SetClipping(EWidgetClipping::ClipToBounds);
	Cover->AddChild(Image);
	AddFullScreen(Cover);
	return Cover;
}

void ULWMenuWidget::AddMotes(UCanvasPanel* Root)
{
	ULWMoteField* Motes = WidgetTree->ConstructWidget<ULWMoteField>();
	Motes->SetVisibility(ESlateVisibility::HitTestInvisible);
	AddFullScreen(Motes);
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

	if (BackdropImage.IsValid())
	{
		UTexture2D* Art = PickBackdropArt(LWNeon::GetViewAspect(GetWorld()));
		if (Art && Art != BackdropArt.Get())
		{
			SetBackdropArt(Art);
		}
	}

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
			Button->SetBackgroundColor(WithAlpha(bActive ? FLinearColor(Pulse, Pulse, Pulse) : FLinearColor::White, Item.bLocked ? (bActive ? 0.6f : 0.4f) : 1.f));
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
	// so the intro is the notices, then the title, then the menu takes over.
	constexpr float NoticeIn = 0.3f, NoticeHold = 9.f, NoticeOut = 9.6f;
	constexpr float TitleIn = 9.9f, TitleHold = 12.2f, TitleOut = 13.f;

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
	Backdrop = AddBackdrop(Root);
	bArtBackdrop = Backdrop != nullptr;
	if (!Backdrop)
	{
		UBorder* Black = WidgetTree->ConstructWidget<UBorder>();
		Black->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 1.f));
		AddFullScreen(Black);
		Backdrop = Black;
	}
	AddMotes(Root);

	NoticeVeil = AddVeil(Root, 0.94f);

	UVerticalBox* Notices = AddColumn(Root, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), 1180.f);
	Notice = Notices;
	Notice->SetRenderOpacity(0.f);
	auto AddNotice = [&](const TCHAR* Text, int32 Size, const FLinearColor& Color, float Below, int32 Spacing)
	{
		UTextBlock* Block = MakeBody(Text, Size, Color, Spacing);
		Block->SetAutoWrapText(true);
		if (UVerticalBoxSlot* BlockSlot = Notices->AddChildToVerticalBox(Block))
		{
			BlockSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Below));
		}
	};
	AddNotice(TEXT("PHOTOSENSITIVITY WARNING"), 30, LWStyle::Gold, 18.f, 260);
	AddNotice(TEXT("This game contains flashing lights and moving patterns. A very small percentage of people may\n")
		TEXT("experience seizures when exposed to them, even with no history of epilepsy."), 22, LWStyle::Text, 16.f, 20);
	AddNotice(TEXT("Stop playing immediately and consult a doctor if you experience light-headedness,\naltered vision, twitching, disorientation or any loss of awareness."),
		22, LWStyle::Text, 16.f, 20);
	AddNotice(TEXT("Screen flashes and screen shake can be turned off in Settings.\nThis reduces flashing but does not remove it completely."), 22, LWStyle::Text, 52.f, 20);
	// Broken by hand, so no line is left with a single word on it.
	AddNotice(TEXT("LightWalker uses Unreal\u00AE Engine.\nUnreal\u00AE is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.\n")
		TEXT("Unreal\u00AE Engine, Copyright 1998\u20132026, Epic Games, Inc. All rights reserved."), 17, LWStyle::Muted, 18.f, 20);
	AddNotice(TEXT("The Audiowide and Rajdhani fonts are used under the SIL Open Font License, Version 1.1."), 17, LWStyle::Muted, 18.f, 20);
	AddNotice(TEXT("This game is a work of fiction. Any resemblance to real people or events is coincidental.\nUnauthorised copying, distribution or sale of this game is prohibited."),
		17, LWStyle::Muted, 0.f, 20);
	Legal = AddCopyright(Root);
	Legal->SetRenderOpacity(0.f);

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

	Notice->SetRenderOpacity(FadeWindow(Time, NoticeIn, NoticeHold, NoticeOut));
	NoticeVeil->SetRenderOpacity(1.f - FMath::Clamp((Time - NoticeHold) / (TitleIn - NoticeHold), 0.f, 1.f));
	Title->SetRenderOpacity(FadeWindow(Time, TitleIn, TitleHold, TitleOut, 1.f));
	Legal->SetRenderOpacity(FMath::Clamp((Time - NoticeIn) / 0.6f, 0.f, 1.f));
	// Over the art, only the logo fades; the sunset stays for the main menu.
	if (!bArtBackdrop)
	{
		// The title shows over the arena: the black backdrop lifts as it fades in.
		Backdrop->SetRenderOpacity(1.f - FMath::Clamp((Time - TitleIn) / 2.f, 0.f, 1.f));
	}

	if (Time >= TitleOut)
	{
		Finish();
	}
}

void ULWIntroScreen::Skip()
{
	if (Time < NoticeHold)
	{
		Time = NoticeHold;  // on to the title
	}
	else if (Time > TitleIn)
	{
		Finish();
	}
}

void ULWIntroScreen::Finish()
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
	// The neon sunset with motes of light rising through it (over the arena if the art isn't imported).
	AddBackdrop(Root);
	AddMotes(Root);

	// The logo, top centre.
	UWidget* Logo = MakeLogo(640.f, 96);
	UCanvasPanelSlot* LogoSlot = Root->AddChildToCanvas(Logo);
	LogoSlot->SetAnchors(FAnchors(0.5f, 0.f));
	LogoSlot->SetAlignment(FVector2D(0.5f, 0.f));
	LogoSlot->SetPosition(FVector2D(0.f, 24.f));
	LogoSlot->SetAutoSize(true);
	AddSlideIn(Logo);

	UVerticalBox* Column = AddColumn(Root, FVector2D(0.07f, 0.55f), FVector2D(0.f, 0.5f), 420.f);
	AddButton(Column, TEXT("PLAY"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->StartGameFromMenu(ERunMode::Story); } });
	EndlessButton = AddButton(Column, TEXT("ENDLESS"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->StartGameFromMenu(ERunMode::Endless); } });
	HeroButton = AddButton(Column, TEXT("HERO"), [this]()
	{
		if (ALightWalkerPlayerController* Controller = GetLWController())
		{
			Controller->CycleHero();
			RefreshHeroLabel();
		}
	});
	AddButton(Column, TEXT("LEADERBOARD"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->OpenLeaderboard(); } });
	AddButton(Column, TEXT("SETTINGS"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->OpenSettings(); } });
	AddButton(Column, TEXT("QUIT"), [this]() { if (ALightWalkerPlayerController* Controller = GetLWController()) { Controller->QuitGame(); } });
	RefreshHeroLabel();

	// Beside the endless button while it's locked (NativeTick).
	Canvas = Root;
	Hint = MakeBody(TEXT("DEFEAT THE ONE-EYED LIAR TO UNLOCK"), 22, LWStyle::Gold, 120);
	Hint->SetRenderOpacity(0.f);
	Hint->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* HintSlot = Root->AddChildToCanvas(Hint);
	HintSlot->SetAlignment(FVector2D(0.f, 0.5f));
	HintSlot->SetAutoSize(true);

	UVerticalBox* Footer = AddColumn(Root, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f), 1400.f, FVector2D(0.f, -58.f));
	Footer->AddChildToVerticalBox(MakeBody(TEXT("ENTER / A  SELECT       ESC / B  BACK"), 18, LWStyle::Muted, 300));
	AddCopyright(Root);
}

void ULWMainMenu::NativeConstruct()
{
	Super::NativeConstruct();
	ULWScoreboard* Scores = ULWScoreboard::Get(this);
	SetButtonLocked(EndlessButton, Scores && !Scores->IsEndlessUnlocked());
	HintKnock = 0.f;
}

void ULWMainMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const bool bShown = IsButtonLocked(EndlessButton) && (EndlessButton->HasKeyboardFocus() || EndlessButton->HasAnyUserFocus());
	Hint->SetRenderOpacity(FMath::FInterpTo(Hint->GetRenderOpacity(), bShown ? 1.f : 0.f, InDeltaTime, 12.f));
	// Selecting it knocks: the hint flares and shudders.
	HintKnock = FMath::Max(0.f, HintKnock - InDeltaTime * 2.5f);
	Hint->SetColorAndOpacity(FSlateColor(FMath::Lerp(LWStyle::Gold, LWStyle::Pink, HintKnock)));
	Hint->SetRenderTranslation(FVector2D(FMath::Sin(HintKnock * 30.f) * 8.f * HintKnock, 0.f));

	const FGeometry& At = EndlessButton->GetCachedGeometry();
	const FVector2D Beside = Canvas->GetCachedGeometry().AbsoluteToLocal(At.LocalToAbsolute(FVector2D(At.GetLocalSize()) * FVector2D(1.f, 0.5f)));
	if (UCanvasPanelSlot* HintSlot = Cast<UCanvasPanelSlot>(Hint->Slot))
	{
		HintSlot->SetPosition(Beside + FVector2D(26.f, 0.f));
	}
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
			S->SetGraphicsLevel((FMath::Clamp(S->GetGraphicsLevel(), 0, 3) + 1) % 4);
			S->ApplyNonResolutionSettings();
			Refresh();
		}
	});

	WindowButton = AddButton(Left, TEXT("DISPLAY"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			const EWindowMode::Type Mode = S->GetFullscreenMode();
			if (Mode == EWindowMode::WindowedFullscreen)
			{
				// On to exclusive fullscreen at the size the display is really showing.
				S->SetScreenResolution(ViewSize(GetWorld()));
			}
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

	// HDR output, on displays that support it (Windows HDR on, DX12). Saved with the other settings on Back.
	HDRButton = AddButton(Left, TEXT("HDR"), [this]()
	{
		ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get();
		if (S && S->SupportsHDRDisplayOutput())
		{
			S->EnableHDRDisplayOutput(!S->IsHDREnabled(), 1000);
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

	FlashButton = AddButton(Right, TEXT("SCREEN FLASHES"), [this]()
	{
		if (ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get())
		{
			S->bScreenFlashes = !S->bScreenFlashes;
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

void ULWSettingsMenu::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();  // the screen may have changed since it was last open
}

void ULWSettingsMenu::Refresh()
{
	const ULightWalkerUserSettings* S = ULightWalkerUserSettings::Get();
	if (!S)
	{
		return;
	}
	// Borderless always fills the display, whatever resolution is saved.
	const FIntPoint Resolution = S->GetFullscreenMode() == EWindowMode::WindowedFullscreen ? ViewSize(GetWorld()) : S->GetScreenResolution();
	SetButtonLabel(QualityButton, FString::Printf(TEXT("GRAPHICS: %s"), *QualityName(S->GetGraphicsLevel())));
	SetButtonLabel(WindowButton, FString::Printf(TEXT("DISPLAY: %s"), *WindowName(S->GetFullscreenMode())));
	SetButtonLabel(ResolutionButton, FString::Printf(TEXT("RESOLUTION: %d x %d"), Resolution.X, Resolution.Y));
	const float FrameRate = S->GetFrameRateLimit();
	SetButtonLabel(FrameRateButton, FrameRate > 0.f ? FString::Printf(TEXT("FRAME RATE: %d FPS"), FMath::RoundToInt(FrameRate)) : FString(TEXT("FRAME RATE: UNLIMITED")));
	SetButtonLabel(VSyncButton, FString::Printf(TEXT("V-SYNC: %s"), S->IsVSyncEnabled() ? TEXT("ON") : TEXT("OFF")));
	SetButtonLabel(HDRButton, !S->SupportsHDRDisplayOutput() ? FString(TEXT("HDR: NOT AVAILABLE")) : FString::Printf(TEXT("HDR: %s"), S->IsHDREnabled() ? TEXT("ON") : TEXT("OFF")));
	SetButtonLabel(VibrationButton, FString::Printf(TEXT("VIBRATION: %s"), S->bVibration ? TEXT("ON") : TEXT("OFF")));
	SetButtonLabel(ShakeButton, FString::Printf(TEXT("SCREEN SHAKE: %s"), S->bScreenShake ? TEXT("ON") : TEXT("OFF")));
	SetButtonLabel(FlashButton, FString::Printf(TEXT("SCREEN FLASHES: %s"), S->bScreenFlashes ? TEXT("ON") : TEXT("OFF")));
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

	// The longest page: rows sit closer together so the title stays on screen.
	for (UPanelSlot* Row : Column->GetSlots())
	{
		UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(Row);
		if (RowSlot && Cast<UButton>(RowSlot->Content))
		{
			RowSlot->SetPadding(FMargin(0.f, 1.f));
		}
	}

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

// ---------------------------------------------------------------------------------------------------------------
// Rising motes of light

void ULWMoteField::NativeConstruct()
{
	Super::NativeConstruct();

	if (UTexture2D* Glow = LWStyle::Art(TEXT("SoftGlow")))
	{
		Brush = LWStyle::ArtBrush(Glow, FVector2D(64.f, 64.f));
	}
	if (Motes.Num() == 0)
	{
		Motes.SetNum(MoteCount);
		for (FMote& Mote : Motes)
		{
			Respawn(Mote, true);
		}
	}
}

void ULWMoteField::Respawn(FMote& Mote, bool bAnywhere)
{
	// Mostly small warm sparks; a few big, faint, out-of-focus ones nearer the eye.
	const bool bBokeh = FMath::FRand() < 0.12f;
	Mote.Position = FVector2D(FMath::FRand(), bAnywhere ? FMath::FRandRange(0.05f, 1.05f) : FMath::FRandRange(1.02f, 1.1f));
	Mote.Size = bBokeh ? FMath::FRandRange(110.f, 180.f) : FMath::FRandRange(26.f, 64.f);
	Mote.Brightness = bBokeh ? FMath::FRandRange(0.12f, 0.22f) : FMath::FRandRange(0.6f, 1.f);
	// Nearer (bigger) motes rise faster.
	Mote.Speed = bBokeh ? FMath::FRandRange(0.05f, 0.08f) : FMath::FRandRange(0.018f, 0.055f);
	Mote.SwayAmount = FMath::FRandRange(6.f, 22.f);
	Mote.SwayRate = FMath::FRandRange(0.35f, 1.1f);
	Mote.Twinkle = FMath::FRandRange(1.5f, 4.f);
	Mote.Phase = FMath::FRand() * UE_TWO_PI;

	// The sunset's palette: lantern gold, with some pink and a little cyan.
	const float Pick = FMath::FRand();
	Mote.Color = Pick < 0.6f ? FLinearColor(1.f, 0.78f, 0.45f) : Pick < 0.85f ? FLinearColor(1.f, 0.4f, 0.72f) : FLinearColor(0.4f, 0.9f, 1.f);
}

void ULWMoteField::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	Time += InDeltaTime;
	for (FMote& Mote : Motes)
	{
		Mote.Position.Y -= Mote.Speed * InDeltaTime;
		if (Mote.Position.Y < -0.05f)
		{
			Respawn(Mote, false);
		}
	}
}

int32 ULWMoteField::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
	if (!Brush.GetResourceObject())
	{
		return Layer;
	}

	const FVector2D Area = AllottedGeometry.GetLocalSize();
	const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
	for (const FMote& Mote : Motes)
	{
		// Fades in as it leaves the floor and dies away towards the top, twinkling on the way.
		const float Y = Mote.Position.Y;
		const float Fade = FMath::Clamp((1.05f - Y) / 0.15f, 0.f, 1.f) * FMath::Clamp((Y + 0.02f) / 0.45f, 0.f, 1.f);
		const float Twinkle = 0.7f + 0.3f * FMath::Sin(Time * Mote.Twinkle + Mote.Phase);
		const float Alpha = Mote.Brightness * Fade * Twinkle;
		if (Alpha < 0.01f)
		{
			continue;
		}

		const FVector2f Size(Mote.Size, Mote.Size);
		const FVector2f Centre(
			Mote.Position.X * Area.X + Mote.SwayAmount * FMath::Sin(Time * Mote.SwayRate + Mote.Phase),
			Y * Area.Y);
		FLinearColor Color = Mote.Color * Tint;
		Color.A = Alpha * Tint.A;
		FSlateDrawElement::MakeBox(OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Centre - Size * 0.5f)), &Brush, ESlateDrawEffect::None, Color);
	}
	return Layer;
}
