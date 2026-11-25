#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"
#include "MoviePlayer.h"
#include "Widgets/SWindow.h"

/**
 * Loaded before the startup movie (the game module is too late for that). The movie player sizes the movie in real
 * pixels, but Windows' display scaling is then applied on top, so at 125% on a wide screen it came out a quarter too
 * wide. The scaling is cancelled out for as long as the movie plays.
 */
class FLightWalkerBoot : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
		if (!IsMoviePlayerEnabled())
		{
			return;
		}
		GetMoviePlayer()->OnMoviePlaybackStarted().AddLambda([]()
		{
			TArray<TSharedRef<SWindow>> Windows;
			FSlateApplication::Get().GetAllVisibleWindowsOrdered(Windows);
			const float Scale = Windows.Num() > 0 ? Windows[0]->GetDPIScaleFactor() : 1.f;
			FSlateApplication::Get().SetApplicationScale(Scale > 0.f ? 1.f / Scale : 1.f);
		});
		GetMoviePlayer()->OnMoviePlaybackFinished().AddLambda([]()
		{
			FSlateApplication::Get().SetApplicationScale(1.f);
		});
	}
};

IMPLEMENT_MODULE(FLightWalkerBoot, LightWalkerBoot);
