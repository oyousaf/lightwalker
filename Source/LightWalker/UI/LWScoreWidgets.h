#pragma once

#include "CoreMinimal.h"
#include "Core/LightWalkerGameInstance.h"
#include "UI/LWMenuWidgets.h"
#include "LWScoreWidgets.generated.h"

/**
 * A score that made the leaderboard: type a name, or pick letters with the d-pad or stick (up and down change the
 * letter, left and right move along).
 */
UCLASS()
class LIGHTWALKER_API ULWNameEntry : public ULWMenuWidget
{
	GENERATED_BODY()

public:
	TFunction<void(const FString&)> OnDone;

	/** Call before showing it. */
	void Begin(int32 Score, const FString& Name);

protected:
	virtual void Build(UCanvasPanel* Root) override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void Refresh();
	void Finish();
	void Step(int32 Change);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> Letters;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ScoreText;

	/** One character per place; a space is empty. */
	FString Entry;
	int32 Cursor = 0;
	int32 Score = 0;
	/** Ignores whatever was still held from the fight. */
	float Age = 0.f;
	bool bFinished = false;
};

/** The ten best runs, story or endless. After a run it shows where it came, and offers another. */
UCLASS()
class LIGHTWALKER_API ULWLeaderboard : public ULWMenuWidget
{
	GENERATED_BODY()

public:
	TFunction<void()> OnBack;
	TFunction<void()> OnPlayAgain;

	/** Which board, the line to pick out (-1 for none), and whether a run has just ended. Call before showing it. */
	void Begin(ERunMode InMode, int32 InHighlight, bool bInRunEnd, int32 InScore);

protected:
	virtual void Build(UCanvasPanel* Root) override;
	virtual void NativeConstruct() override;

private:
	void Refresh();
	bool IsReady() const;

	struct FRow
	{
		TWeakObjectPtr<UTextBlock> Place;
		TWeakObjectPtr<UTextBlock> Name;
		TWeakObjectPtr<UTextBlock> Reached;
		TWeakObjectPtr<UTextBlock> Score;
	};
	TArray<FRow> Rows;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Heading;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ModeButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AgainButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BackButton;

	ERunMode Mode = ERunMode::Endless;
	ERunMode RunMode = ERunMode::Endless;
	int32 Highlight = -1;
	int32 RunScore = 0;
	bool bRunEnd = false;
	double ShownAt = 0.0;
};
