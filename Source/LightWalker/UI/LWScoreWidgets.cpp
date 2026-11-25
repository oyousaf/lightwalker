#include "UI/LWScoreWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/LWScoreboard.h"
#include "Core/LWSound.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UI/LWStyle.h"

namespace
{
	const FString Alphabet = TEXT(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
	/** Presses before this are left over from the fight. */
	constexpr float EntryGuard = 0.7f;
}

// ---------------------------------------------------------------------------------------------------------------
// Name entry

void ULWNameEntry::Begin(int32 InScore, const FString& Name)
{
	Score = InScore;
	Entry = ULWScoreboard::CleanName(Name).RightPad(ULWScoreboard::NameLength);
	Cursor = FMath::Min(Entry.TrimEnd().Len(), ULWScoreboard::NameLength - 1);
	Age = 0.f;
	bFinished = false;
	Refresh();
}

void ULWNameEntry::Build(UCanvasPanel* Root)
{
	AddVeil(Root, 0.62f);
	UVerticalBox* Column = AddPanel(Root, 900.f);
	Column->AddChildToVerticalBox(MakeTitle(TEXT("HIGH SCORE"), 56));
	ScoreText = MakeBody(TEXT(""), 44, LWStyle::Gold, 120);
	if (UVerticalBoxSlot* ScoreSlot = Column->AddChildToVerticalBox(ScoreText))
	{
		ScoreSlot->SetPadding(FMargin(0.f, 96.f, 0.f, 10.f));
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	for (int32 Index = 0; Index < ULWScoreboard::NameLength; ++Index)
	{
		UTextBlock* Letter = MakeText(TEXT("_"), 46, LWStyle::Text);
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(68.f);
		Box->AddChild(Letter);
		Row->AddChildToHorizontalBox(Box);
		Letters.Add(Letter);
	}
	if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Row))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 30.f));
	}
	Column->AddChildToVerticalBox(MakeBody(TEXT("TYPE YOUR NAME, THEN ENTER"), 22, LWStyle::Muted, 200));
	if (UVerticalBoxSlot* HintSlot = Column->AddChildToVerticalBox(MakeBody(TEXT("GAMEPAD: UP AND DOWN CHANGE THE LETTER, A TO FINISH"), 22, LWStyle::Muted, 200)))
	{
		HintSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
	}
	AddButton(Column, TEXT("DONE"), [this]() { Finish(); });
	Refresh();
}

void ULWNameEntry::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

void ULWNameEntry::Refresh()
{
	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(FString::Printf(TEXT("%06d"), Score)));
	}
	for (int32 Index = 0; Index < Letters.Num(); ++Index)
	{
		const TCHAR Character = Entry.IsValidIndex(Index) ? Entry[Index] : TEXT(' ');
		Letters[Index]->SetText(FText::FromString(Character == TEXT(' ') ? FString(TEXT("_")) : FString::Chr(Character)));
		Letters[Index]->SetColorAndOpacity(FSlateColor(Index == Cursor ? LWStyle::Pink : (Character == TEXT(' ') ? LWStyle::WithAlpha(LWStyle::Muted, 0.5f) : LWStyle::Text)));
	}
}

void ULWNameEntry::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
#if !UE_BUILD_SHIPPING
	// Dev only: -LWAutoName enters a name by itself, to test the leaderboard.
	static const bool bAutoName = FParse::Param(FCommandLine::Get(), TEXT("LWAutoName"));
	if (bAutoName && Age > 4.f)
	{
		Finish();
	}
#endif
	if (Letters.IsValidIndex(Cursor))
	{
		// The place being edited blinks.
		Letters[Cursor]->SetRenderOpacity(0.55f + 0.45f * FMath::Cos(Age * 7.f));
	}
	for (int32 Index = 0; Index < Letters.Num(); ++Index)
	{
		if (Index != Cursor)
		{
			Letters[Index]->SetRenderOpacity(1.f);
		}
	}
}

void ULWNameEntry::Step(int32 Change)
{
	int32 At = INDEX_NONE;
	Alphabet.FindChar(Entry[Cursor], At);
	At = (FMath::Max(At, 0) + Change + Alphabet.Len()) % Alphabet.Len();
	Entry[Cursor] = Alphabet[At];
}

void ULWNameEntry::Finish()
{
	if (bFinished || Age < EntryGuard)
	{
		return;
	}
	bFinished = true;
	ULWSoundSubsystem::Play(this, ELWSound::UISelect);
	if (OnDone)
	{
		OnDone(Entry);
	}
}

FReply ULWNameEntry::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Every key is the name's here: none of them pause, fire or go back.
	const FKey Key = InKeyEvent.GetKey();
	if (Age < EntryGuard)
	{
		return FReply::Handled();
	}
	const bool bSubmit = Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_Special_Right;
	if (bSubmit)
	{
		if (!InKeyEvent.IsRepeat())
		{
			Finish();
		}
		return FReply::Handled();
	}

	const int32 Last = ULWScoreboard::NameLength - 1;
	const TCHAR Character = FChar::ToUpper(static_cast<TCHAR>(InKeyEvent.GetCharacter()));
	if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
	{
		Cursor = FMath::Max(0, Cursor - 1);
	}
	else if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
	{
		Cursor = FMath::Min(Last, Cursor + 1);
	}
	else if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		Step(1);
	}
	else if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		Step(-1);
	}
	else if (Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Left || Key == EKeys::Gamepad_FaceButton_Right)
	{
		// Clears this place, or the one before if it's already empty.
		if (Entry[Cursor] == TEXT(' '))
		{
			Cursor = FMath::Max(0, Cursor - 1);
		}
		Entry[Cursor] = TEXT(' ');
	}
	else if (!Key.IsGamepadKey() && ((Character >= TEXT('A') && Character <= TEXT('Z')) || (Character >= TEXT('0') && Character <= TEXT('9'))))
	{
		Entry[Cursor] = Character;
		Cursor = FMath::Min(Last, Cursor + 1);
	}
	else
	{
		return FReply::Handled();
	}
	ULWSoundSubsystem::Play(this, ELWSound::UIMove);
	Refresh();
	return FReply::Handled();
}

// ---------------------------------------------------------------------------------------------------------------
// Leaderboard

void ULWLeaderboard::Begin(ERunMode InMode, int32 InHighlight, bool bInRunEnd, int32 InScore)
{
	Mode = RunMode = InMode;
	Highlight = InHighlight;
	bRunEnd = bInRunEnd;
	RunScore = InScore;
	ShownAt = FPlatformTime::Seconds();
	Refresh();
}

void ULWLeaderboard::Build(UCanvasPanel* Root)
{
	AddVeil(Root, 0.62f);
	UVerticalBox* Column = AddPanel(Root, 1060.f);
	if (UBorder* Panel = Cast<UBorder>(Column->GetParent()->GetParent()))
	{
		Panel->SetPadding(FMargin(90.f, 64.f, 90.f, 225.f));
	}
	Column->AddChildToVerticalBox(MakeTitle(TEXT("LEADERBOARD"), 48));
	Heading = MakeBody(TEXT(""), 26, LWStyle::Cyan, 200);
	if (UVerticalBoxSlot* HeadingSlot = Column->AddChildToVerticalBox(Heading))
	{
		HeadingSlot->SetPadding(FMargin(0.f, 92.f, 0.f, 8.f));
	}

	for (int32 Index = 0; Index < ULWScoreboard::Size; ++Index)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>();
		auto AddCell = [&](float Share, ETextJustify::Type Align)
		{
			UTextBlock* Cell = MakeBody(TEXT(""), 24, LWStyle::Text, 60);
			Cell->SetJustification(Align);
			if (UHorizontalBoxSlot* CellSlot = Line->AddChildToHorizontalBox(Cell))
			{
				FSlateChildSize CellSize(ESlateSizeRule::Fill);
				CellSize.Value = Share;
				CellSlot->SetSize(CellSize);
			}
			return Cell;
		};
		FRow& Row = Rows.AddDefaulted_GetRef();
		Row.Place = AddCell(0.12f, ETextJustify::Left);
		Row.Name = AddCell(0.34f, ETextJustify::Left);
		Row.Reached = AddCell(0.32f, ETextJustify::Left);
		Row.Score = AddCell(0.22f, ETextJustify::Right);
		if (UVerticalBoxSlot* LineSlot = Column->AddChildToVerticalBox(Line))
		{
			LineSlot->SetPadding(FMargin(60.f, 0.f));
		}
	}

	// The buttons side by side along the bottom, so ten lines fit.
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (UVerticalBoxSlot* ButtonsSlot = Column->AddChildToVerticalBox(Buttons))
	{
		ButtonsSlot->SetPadding(FMargin(30.f, 10.f, 30.f, 0.f));
	}
	AgainButton = AddButton(Buttons, TEXT("PLAY AGAIN"), [this]() { if (OnPlayAgain && IsReady()) { OnPlayAgain(); } });
	ModeButton = AddButton(Buttons, TEXT("STORY"), [this]()
	{
		Mode = Mode == ERunMode::Endless ? ERunMode::Story : ERunMode::Endless;
		Refresh();
	});
	BackButton = AddButton(Buttons, TEXT("BACK"), [this]() { if (OnBack && IsReady()) { OnBack(); } });
	for (UButton* Button : { AgainButton.Get(), ModeButton.Get(), BackButton.Get() })
	{
		if (UHorizontalBoxSlot* ButtonSlot = Cast<UHorizontalBoxSlot>(Button->Slot))
		{
			ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ButtonSlot->SetPadding(FMargin(6.f, 0.f));
		}
	}
	Refresh();
}

void ULWLeaderboard::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

bool ULWLeaderboard::IsReady() const
{
	// Presses left over from the fight don't skip it.
	return !bRunEnd || FPlatformTime::Seconds() - ShownAt > 0.6;
}

void ULWLeaderboard::Refresh()
{
	ULWScoreboard* Scores = ULWScoreboard::Get(this);
	if (!Scores || Rows.Num() == 0)
	{
		return;
	}

	const bool bEndless = Mode == ERunMode::Endless;
	FString Title = bEndless ? TEXT("ENDLESS") : TEXT("STORY");
	if (bEndless && !Scores->IsEndlessUnlocked())
	{
		Title += TEXT("     DEFEAT THE ONE-EYED LIAR TO UNLOCK");
	}
	else if (bRunEnd)
	{
		Title += FString::Printf(TEXT("     YOUR SCORE  %06d"), RunScore);
	}
	Heading->SetText(FText::FromString(Title));
	SetButtonLabel(ModeButton, bEndless ? TEXT("SEE STORY") : TEXT("SEE ENDLESS"));
	SetButtonLabel(BackButton, bRunEnd ? TEXT("MAIN MENU") : TEXT("BACK"));
	AgainButton->SetVisibility(bRunEnd ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	const TArray<FLWScore>& Board = Scores->GetBoard(Mode);
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const bool bFilled = Board.IsValidIndex(Index);
		const bool bYours = bFilled && bRunEnd && Mode == RunMode && Index == Highlight;
		const FLinearColor Colour = bYours ? LWStyle::Gold : (bFilled ? LWStyle::Text : LWStyle::WithAlpha(LWStyle::Muted, 0.35f));
		auto Set = [&](const TWeakObjectPtr<UTextBlock>& Cell, const FString& Text)
		{
			if (Cell.IsValid())
			{
				Cell->SetText(FText::FromString(Text));
				Cell->SetColorAndOpacity(FSlateColor(Colour));
			}
		};
		Set(Rows[Index].Place, FString::FromInt(Index + 1));
		Set(Rows[Index].Name, bFilled ? Board[Index].Name : TEXT("-"));
		Set(Rows[Index].Reached, bFilled ? Board[Index].Reached : FString());
		Set(Rows[Index].Score, bFilled ? FString::Printf(TEXT("%06d"), Board[Index].Score) : FString());
	}
}
