#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LWMenuWidgets.generated.h"

class ALightWalkerPlayerController;
class UButton;
class UCanvasPanel;
class UPanelWidget;
class USlider;
class UTextBlock;
class UVerticalBox;

/** Forwards a button click to a lambda, so code-built menus need no UFUNCTION per button. */
UCLASS()
class ULWClickRelay : public UObject
{
	GENERATED_BODY()

public:
	TFunction<void()> Callback;

	UFUNCTION()
	void HandleClicked() { if (Callback) { Callback(); } }
};

/** Forwards a slider change to a lambda. */
UCLASS()
class ULWSliderRelay : public UObject
{
	GENERATED_BODY()

public:
	TFunction<void(float)> Callback;

	UFUNCTION()
	void HandleValueChanged(float Value) { if (Callback) { Callback(Value); } }
};

/** Soft motes of light drifting up the screen like embers, drawn behind the menus' buttons. */
UCLASS()
class LIGHTWALKER_API ULWMoteField : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	struct FMote
	{
		/** X across the screen, Y down it, both 0 to 1. */
		FVector2D Position = FVector2D::ZeroVector;
		/** Screen heights per second. */
		float Speed = 0.f;
		float Size = 0.f;
		float Brightness = 0.f;
		float SwayAmount = 0.f;
		float SwayRate = 0.f;
		float Twinkle = 0.f;
		float Phase = 0.f;
		FLinearColor Color = FLinearColor::White;
	};

	/** A new mote just below the bottom (or anywhere, when the field first fills). */
	static void Respawn(FMote& Mote, bool bAnywhere);

	static constexpr int32 MoteCount = 90;
	TArray<FMote> Motes;
	FSlateBrush Brush;
	float Time = 0.f;
};

/**
 * Base for the menus. Each one builds its widget tree in code (the game needs no widget assets), with neon
 * buttons that light up when hovered or focused, so mouse, keyboard and gamepad all show where you are.
 */
UCLASS(Abstract)
class LIGHTWALKER_API ULWMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Puts keyboard and gamepad focus on the first button (or the one at Index). */
	void FocusButton(int32 Index = 0);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Builds the tree under the canvas root. */
	virtual void Build(UCanvasPanel* Root) {}

	ALightWalkerPlayerController* GetLWController() const;

	/** Display face (Audiowide). */
	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color, int32 LetterSpacing = 0);
	/** Text face (Rajdhani). */
	UTextBlock* MakeBody(const FString& Text, int32 Size, const FLinearColor& Color, int32 LetterSpacing = 0);
	/** Big pink neon title with a cyan echo, leaning forward. */
	UTextBlock* MakeTitle(const FString& Text, int32 Size);
	/** The LIGHTWALKER logo art at the given width, or the neon title text if the art isn't imported. */
	UWidget* MakeLogo(float Width, int32 FallbackTextSize);
	UButton* AddButton(UPanelWidget* Parent, const FString& Label, TFunction<void()> OnClick);
	void AddSlider(UPanelWidget* Parent, const FString& Label, float Value, TFunction<void(float)> OnChange);
	void SetButtonLabel(UButton* Button, const FString& Label);

	/** A centred column of the given width, for the menu's title and buttons. */
	UVerticalBox* AddColumn(UCanvasPanel* Root, const FVector2D& Anchor, const FVector2D& Alignment, float Width, const FVector2D& Offset = FVector2D::ZeroVector);

	/** A centred dark glass panel with a neon edge, holding a column (pause, settings). */
	UVerticalBox* AddPanel(UCanvasPanel* Root, float Width);

	/** The game window's width over height. */
	float GetViewAspect() const;

	/** Adds a widget covering the whole screen (past the 16:9 area on ultrawide), behind the menu's content. */
	void AddFullScreen(UWidget* Widget);

	/** A full-screen dark veil behind overlay menus (pause, settings). */
	void AddVeil(UCanvasPanel* Root, float Opacity);

	/** The full-screen neon sunset art; null (and nothing added) if it isn't imported. */
	UWidget* AddBackdrop(UCanvasPanel* Root);

	/** Motes of light rising across the whole screen. */
	void AddMotes(UCanvasPanel* Root);

	/** Makes a widget slide in and fade up with the menu. */
	void AddSlideIn(UWidget* Widget) { SlideIns.Add(Widget); }


private:
	struct FFocusable
	{
		TWeakObjectPtr<UWidget> Widget;
		TWeakObjectPtr<UButton> Button;
		TWeakObjectPtr<UTextBlock> Label;
		bool bWasActive = false;
	};
	TArray<FFocusable> Focusables;

	/** Columns and panels that slide in when the menu opens. */
	TArray<TWeakObjectPtr<UWidget>> SlideIns;

	/** The whole screen; Build's Root is a centred area inside it no wider than 16:9. */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> FullScreen;
	int32 FullScreenLayers = 0;

	/** The sunset art, and which version of it (16:9, 21:9 or 32:9) it shows. */
	static UTexture2D* PickBackdropArt(float Aspect);
	void SetBackdropArt(UTexture2D* Art);
	TWeakObjectPtr<class UImage> BackdropImage;
	TWeakObjectPtr<UTexture2D> BackdropArt;
	float AppearTime = 0.f;
	TWeakObjectPtr<UWidget> LastFocused;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Relays;
};

/** After Epic's logo movie: the title rises out of black over the arena, then hands over to the main menu. */
UCLASS()
class LIGHTWALKER_API ULWIntroScreen : public ULWMenuWidget
{
	GENERATED_BODY()

public:
	TFunction<void()> OnFinished;

	/** Any key or click skips ahead. */
	void Skip();

protected:
	virtual void Build(UCanvasPanel* Root) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWidget> Title;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Backdrop;

	bool bArtBackdrop = false;

	float Time = 0.f;
	bool bFinished = false;
};

/** The logo, then Play, Hero, Settings and Quit over the neon sunset, with motes of light rising through it. */
UCLASS()
class LIGHTWALKER_API ULWMainMenu : public ULWMenuWidget
{
	GENERATED_BODY()

protected:
	virtual void Build(UCanvasPanel* Root) override;

private:
	void RefreshHeroLabel();

	UPROPERTY(Transient)
	TObjectPtr<UButton> HeroButton;
};

UCLASS()
class LIGHTWALKER_API ULWPauseMenu : public ULWMenuWidget
{
	GENERATED_BODY()

protected:
	virtual void Build(UCanvasPanel* Root) override;
};

/** Graphics, display, volume and vibration. Changes apply straight away and are saved on Back. */
UCLASS()
class LIGHTWALKER_API ULWSettingsMenu : public ULWMenuWidget
{
	GENERATED_BODY()

public:
	TFunction<void()> OnBack;

protected:
	virtual void Build(UCanvasPanel* Root) override;

private:
	void Refresh();

	UPROPERTY(Transient)
	TObjectPtr<UButton> QualityButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> WindowButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResolutionButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> FrameRateButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> VSyncButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> HDRButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> VibrationButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ShakeButton;
};

/**
 * Settings > Controls: pick a control, then press the key or gamepad button for it (Esc cancels). Keys replace
 * the keyboard binding and buttons the gamepad one, so both stay bound. Taking a key from another control swaps.
 */
UCLASS()
class LIGHTWALKER_API ULWControlsMenu : public ULWMenuWidget
{
	GENERATED_BODY()

protected:
	virtual void Build(UCanvasPanel* Root) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeDestruct() override;

private:
	void Refresh();
	void StopListening();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> BindingButtons;

	/** The control waiting for a key, or -1. */
	int32 Listening = -1;
	/** Ignores the press that started listening. */
	float ListenAge = 0.f;
};
