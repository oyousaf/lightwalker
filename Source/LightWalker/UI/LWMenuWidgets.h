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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Builds the tree under the canvas root. */
	virtual void Build(UCanvasPanel* Root) {}

	ALightWalkerPlayerController* GetLWController() const;

	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color, int32 LetterSpacing = 0);
	UButton* AddButton(UPanelWidget* Parent, const FString& Label, TFunction<void()> OnClick);
	void AddSlider(UPanelWidget* Parent, const FString& Label, float Value, TFunction<void(float)> OnChange);
	void SetButtonLabel(UButton* Button, const FString& Label);

	/** A centred column of the given width, for the menu's title and buttons. */
	UVerticalBox* AddColumn(UCanvasPanel* Root, const FVector2D& Anchor, const FVector2D& Alignment, float Width, const FVector2D& Offset = FVector2D::ZeroVector);

	/** A full-screen dark veil behind overlay menus (pause, settings). */
	void AddVeil(UCanvasPanel* Root, float Opacity);

private:
	struct FFocusable
	{
		TWeakObjectPtr<UWidget> Widget;
		TWeakObjectPtr<UButton> Button;
		TWeakObjectPtr<UTextBlock> Label;
	};
	TArray<FFocusable> Focusables;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Relays;
};

/** First thing on launch: "Powered by Unreal Engine 5", then the title, then it hands over to the main menu. */
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
	TObjectPtr<UWidget> EngineLogo;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EngineLine;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EngineName;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Title;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Backdrop;

	float Time = 0.f;
	bool bFinished = false;
};

/** Title, Play, Hero, Settings, Quit, beside Lightwalker standing in the dark arena. */
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
	TObjectPtr<UButton> VSyncButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> VibrationButton;
};
