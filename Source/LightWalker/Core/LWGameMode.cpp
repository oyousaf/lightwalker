#include "Core/LWGameMode.h"

#include "Camera/PlayerCameraManager.h"
#include "Core/LWFeel.h"
#include "Core/LightWalkerGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Player/LightWalkerPawn.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
	constexpr float TravelFadeTime = 0.7f;
}

ULightWalkerGameInstance* ALWGameMode::GetRun() const
{
	return GetGameInstance<ULightWalkerGameInstance>();
}

ALightWalkerPawn* ALWGameMode::GetLightWalker() const
{
	return Cast<ALightWalkerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
}

bool ALWGameMode::MapExists(const TCHAR* Map)
{
	return FPackageName::DoesPackageExist(Map);
}

void ALWGameMode::TravelTo(const TCHAR* Map, const TCHAR* GameMode)
{
	if (GetWorldTimerManager().IsTimerActive(TravelTimer))
	{
		return;
	}
	if (ULWFeelSubsystem* Feel = GetGameInstance()->GetSubsystem<ULWFeelSubsystem>())
	{
		Feel->CancelHitStop();
	}

	TravelMap = Map;
	TravelOptions = FString::Printf(TEXT("game=%s"), GameMode);
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Camera->StartCameraFade(0.f, 1.f, TravelFadeTime, FLinearColor::Black, false, true);
	}
	GetWorldTimerManager().SetTimer(TravelTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UGameplayStatics::OpenLevel(this, FName(*TravelMap), true, TravelOptions);
	}), TravelFadeTime + 0.05f, false);
}

void ALWGameMode::FadeIn()
{
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Camera->StartCameraFade(1.f, 0.f, TravelFadeTime, FLinearColor::Black, false, false);
	}
}

void ALWGameMode::StartPlay()
{
	Super::StartPlay();

#if !UE_BUILD_SHIPPING
	int32 Shots = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("LWShots="), Shots) && Shots > 0)
	{
		float Every = 3.f;
		FParse::Value(FCommandLine::Get(), TEXT("LWShotEvery="), Every);
		GetWorldTimerManager().SetTimer(DevShotTimer, this, &ALWGameMode::TakeDevScreenshot, Every, true,
			FParse::Param(FCommandLine::Get(), TEXT("LWEarlyShot")) ? 1.5f : 8.f);
	}
#endif
}

#if !UE_BUILD_SHIPPING
void ALWGameMode::TakeDevScreenshot()
{
	// Counted across maps, so a run that travels between modes keeps going.
	static int32 Index = 0;
	int32 Total = 0;
	FParse::Value(FCommandLine::Get(), TEXT("LWShots="), Total);
	if (Index >= Total)
	{
		FPlatformMisc::RequestExit(false);
		return;
	}
	++Index;
	// HighResShot renders the scene only; the menu needs a normal screenshot with the UI.
	if (IsInMenu())
	{
		FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("LWShot_%d"), Index), true, false);
	}
	else if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		Controller->ConsoleCommand(FString::Printf(TEXT("HighResShot 2 filename=LWShot_%d"), Index));
	}
}
#endif
