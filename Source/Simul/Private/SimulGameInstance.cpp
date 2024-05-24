#include "SimulGameInstance.h"

#include "GameFramework/GameModeBase.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameUserSettings.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

ULocalPlayer* USimulGameInstance::CreateInitialPlayer(FString& OutError)
{
	ULocalPlayer* FirstInitialPlayer = Super::CreateInitialPlayer(OutError);

	CreateLocalPlayer(1, OutError, false);

	return FirstInitialPlayer;
}

void USimulGameInstance::Init()
{
	Super::Init();

	UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings();
	GameUserSettings->SetFullscreenMode(EWindowMode::Windowed);
	GameUserSettings->SetScreenResolution({1366,768});
	GameUserSettings->ApplySettings(true);
}

#if WITH_EDITOR

FGameInstancePIEResult USimulGameInstance::StartPlayInEditorGameInstance(ULocalPlayer* LocalPlayer,
                                                                       const FGameInstancePIEParameters& Params)
{
	if (!Params.EditorPlaySettings)
	{
		return FGameInstancePIEResult::Failure(NSLOCTEXT("UnrealEd", "Error_InvalidEditorPlaySettings", "Invalid Editor Play Settings!"));
	}

	if (PIEStartTime == 0)
	{
		PIEStartTime = Params.PIEStartTime;
	}

	BroadcastOnStart();

	UEditorEngine* const EditorEngine = CastChecked<UEditorEngine>(GetEngine());

	// for clients, just connect to the server
	if (Params.NetMode == PIE_Client)
	{
		FString Error;
		FURL BaseURL = WorldContext->LastURL;

		FString URLString(TEXT("127.0.0.1"));
		uint16 ServerPort = 0;
		if (Params.EditorPlaySettings->GetServerPort(ServerPort))
		{
			URLString += FString::Printf(TEXT(":%hu"), ServerPort);
		}

		if (Params.EditorPlaySettings->IsNetworkEmulationEnabled())
		{
			if (Params.EditorPlaySettings->NetworkEmulationSettings.IsEmulationEnabledForTarget(NetworkEmulationTarget::Client))
			{
				URLString += Params.EditorPlaySettings->NetworkEmulationSettings.BuildPacketSettingsForURL();
			}
		}

		if (EditorEngine->Browse(*WorldContext, FURL(&BaseURL, *URLString, (ETravelType)TRAVEL_Absolute), Error) == EBrowseReturnVal::Pending)
		{
			EditorEngine->TransitionType = ETransitionType::WaitingToConnect;
		}
		else
		{
			return FGameInstancePIEResult::Failure(FText::Format(NSLOCTEXT("UnrealEd", "Error_CouldntLaunchPIEClient", "Couldn't Launch PIE Client: {0}"), FText::FromString(Error)));
		}
	}
	else
	{
		// we're going to be playing in the current world, get it ready for play
		UWorld* const PlayWorld = GetWorld();

		FString ExtraURLOptions;
		if (Params.EditorPlaySettings->IsNetworkEmulationEnabled())
		{
			NetworkEmulationTarget CurrentTarget = Params.NetMode == PIE_ListenServer ? NetworkEmulationTarget::Server : NetworkEmulationTarget::Client;
			if (Params.EditorPlaySettings->NetworkEmulationSettings.IsEmulationEnabledForTarget(CurrentTarget))
			{
				ExtraURLOptions += Params.EditorPlaySettings->NetworkEmulationSettings.BuildPacketSettingsForURL();
			}
		}

		// make a URL
		FURL URL;
		// If the user wants to start in spectator mode, do not use the custom play world for now
		if (EditorEngine->UserEditedPlayWorldURL.Len() > 0 || Params.OverrideMapURL.Len() > 0)
		{
			FString UserURL = EditorEngine->UserEditedPlayWorldURL.Len() > 0 ? EditorEngine->UserEditedPlayWorldURL : Params.OverrideMapURL;
			UserURL += ExtraURLOptions;

			// If the user edited the play world url. Verify that the map name is the same as the currently loaded map.
			URL = FURL(NULL, *UserURL, TRAVEL_Absolute);
			if (URL.Map != PIEMapName)
			{
				// Ensure the URL map name is the same as the generated play world map name.
				URL.Map = PIEMapName;
			}
		}
		else
		{
			// The user did not edit the url, just build one from scratch.
			URL = FURL(NULL, *EditorEngine->BuildPlayWorldURL(*PIEMapName, Params.bStartInSpectatorMode, ExtraURLOptions), TRAVEL_Absolute);
		}

		// If a start location is specified, spawn a temporary PlayerStartPIE actor at the start location and use it as the portal.
		AActor* PlayerStart = NULL;
		if (!EditorEngine->SpawnPlayFromHereStart(PlayWorld, PlayerStart))
		{
			// failed to create "play from here" playerstart
			return FGameInstancePIEResult::Failure(NSLOCTEXT("UnrealEd", "Error_FailedCreatePlayFromHerePlayerStart", "Failed to create PlayerStart at desired starting location."));
		}

		if (!PlayWorld->SetGameMode(URL))
		{
			// Setting the game mode failed so bail 
			return FGameInstancePIEResult::Failure(NSLOCTEXT("UnrealEd", "Error_FailedCreateEditorPreviewWorld", "Failed to create editor preview world."));
		}

		FGameInstancePIEResult PostCreateGameModeResult = PostCreateGameModeForPIE(Params, PlayWorld->GetAuthGameMode<AGameModeBase>());
		if (!PostCreateGameModeResult.IsSuccess())
		{
			return PostCreateGameModeResult;
		}
		
		// Make sure "always loaded" sub-levels are fully loaded
		PlayWorld->FlushLevelStreaming(EFlushLevelStreamingType::Visibility);

		PlayWorld->CreateAISystem();

		PlayWorld->InitializeActorsForPlay(URL);
		// calling it after InitializeActorsForPlay has been called to have all potential bounding boxed initialized
		FNavigationSystem::AddNavigationSystemToWorld(*PlayWorld, LocalPlayers.Num() > 0 ? FNavigationSystemRunMode::PIEMode : FNavigationSystemRunMode::SimulationMode);

		// ============================================================================================================
		// Modification: spawning player controller for all local players, default implementation spawns only for the
		// first local player
		// ============================================================================================================
		for(auto It = GetLocalPlayerIterator(); It; ++It)
		{
			FString Error;
			if(!(*It)->SpawnPlayActor(URL.ToString(1),Error, GetWorld()))
			{
				return FGameInstancePIEResult::Failure(FText::Format(NSLOCTEXT("UnrealEd", "Error_CouldntSpawnPlayer", "Couldn't spawn player: {0}"), FText::FromString(Error)));
			}
		}

		UGameViewportClient* const GameViewport = GetGameViewportClient();
		if (GameViewport != NULL && GameViewport->Viewport != NULL)
		{
			// Stream any levels now that need to be loaded before the game starts
			GEngine->BlockTillLevelStreamingCompleted(PlayWorld);
		}
		
		if (Params.NetMode == PIE_ListenServer)
		{
			// Add port
			uint16 ServerPort = 0;
			if (Params.EditorPlaySettings->GetServerPort(ServerPort))
			{
				URL.Port = ServerPort;
			}

			// start listen server with the built URL
			PlayWorld->Listen(URL);
		}

		PlayWorld->BeginPlay();
	}

	// Give the deprecated method a chance to fail as well
	FGameInstancePIEResult StartResult = FGameInstancePIEResult::Success();

	if (StartResult.IsSuccess())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		StartResult = StartPIEGameInstance(LocalPlayer, Params.bSimulateInEditor, Params.bAnyBlueprintErrors, Params.bStartInSpectatorMode) ?
			FGameInstancePIEResult::Success() :
			FGameInstancePIEResult::Failure(NSLOCTEXT("UnrealEd", "Error_CouldntInitInstance", "The game instance failed to Play/Simulate In Editor"));
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	return StartResult;
}

#endif
