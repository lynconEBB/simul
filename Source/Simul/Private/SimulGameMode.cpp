#include "SimulGameMode.h"

#include "EngineUtils.h"
#include "SimulPlayerController.h"
#include "TrainingCharacter/TrainingCharacter.h"
#include "Instructor/InstructorPawn.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/VRPlayerStart.h"
#include "Simulation/SimulationSubsystem.h"

ASimulGameMode::ASimulGameMode()
	: bShowHelpMarkers(false),
	  VRPawnClass(ATrainingCharacter::StaticClass())
{
	DefaultPawnClass = AInstructorPawn::StaticClass();
	PlayerControllerClass = ASimulPlayerController::StaticClass();
}

void ASimulGameMode::SetShowHelperMarkers(bool bShowHelpers)
{
	bShowHelpMarkers = bShowHelpers;
	OnShowHelperUpdate.Broadcast(bShowHelpers);
}

// Spawn Player 0 in VRPlayerStart and Player 1 in default PlayerStart
AActor* ASimulGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Player->IsLocalPlayerController())
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	APlayerController* PlayerControllerTest = Cast<APlayerController>(Player);
	int32 ControllerId = PlayerControllerTest->GetLocalPlayer()->GetControllerId();

	APlayerStart* BestStart = nullptr;
	UClass* DesiredPlayerStartClass = GetDesiredPlayerStartClass(ControllerId);

	USimulationSubsystem* SimulationSubsystem = GetWorld()->GetSubsystem<USimulationSubsystem>();
	FName CurrentSimulationId = NAME_None;
	if (IsValid(SimulationSubsystem))
		CurrentSimulationId = SimulationSubsystem->GetCurrentSimulationId();

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (PlayerStart->IsA(DesiredPlayerStartClass)
			&& (CurrentSimulationId == NAME_None || PlayerStart->Tags.Contains(CurrentSimulationId)))
		{
			BestStart = PlayerStart;
			break;
		}
	}

	return BestStart;
}

UClass* ASimulGameMode::GetDesiredPlayerStartClass(int32 ControllerId)
{
	if (ControllerId == 0)
	{
		return AVRPlayerStart::StaticClass();
	}

	return APlayerStart::StaticClass();
}

// Custom version of Login setting PlayerController LocalPlayer earlier
APlayerController* ASimulGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal,
                                       const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (GameSession == nullptr)
	{
		ErrorMessage = TEXT("Failed to spawn player controller, GameSession is null");
		return nullptr;
	}

	ErrorMessage = GameSession->ApproveLogin(Options);
	if (!ErrorMessage.IsEmpty())
	{
		return nullptr;
	}

	APlayerController* const NewPlayerController = SpawnPlayerController(InRemoteRole, Options);
	if (NewPlayerController == nullptr)
	{
		// Handle spawn failure.
		UE_LOG(LogGameMode, Log, TEXT("Login: Couldn't spawn player controller of class %s"),
		       PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("NULL"));
		ErrorMessage = FString::Printf(TEXT("Failed to spawn player controller"));
		return nullptr;
	}

	// Set here so we can use the controller id to decide the player start
	NewPlayerController->Player = NewPlayer;
	// Customize incoming player based on URL options
	ErrorMessage = InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	if (!ErrorMessage.IsEmpty())
	{
		NewPlayerController->Destroy();
		return nullptr;
	}

	return NewPlayerController;
}


// Spawn different pawns based on Player ID
APawn* ASimulGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	if (!NewPlayer->IsLocalPlayerController())
	{
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}

	APlayerController* PlayerController = Cast<APlayerController>(NewPlayer);
	if (PlayerController->GetLocalPlayer()->GetControllerId() == 0)
	{
		FRotator StartRotation(ForceInit);
		StartRotation.Yaw = StartSpot->GetActorRotation().Yaw;
		FVector StartLocation = StartSpot->GetActorLocation();

		FTransform SpawnTransform = FTransform(StartRotation, StartLocation);

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Instigator = GetInstigator();
		SpawnInfo.ObjectFlags |= RF_Transient;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(VRPawnClass, SpawnTransform, SpawnInfo);

		if (!ResultPawn)
		{
			UE_LOG(LogGameMode, Warning, TEXT("SpawnDefaultPawnAtTransform: Couldn't spawn Pawn of type %s at %s"),
			       *GetNameSafe(VRPawnClass), *SpawnTransform.ToHumanReadableString());
		}
		return ResultPawn;
	}

	if (PlayerController->GetLocalPlayer()->GetControllerId() == 1)
	{
		AInstructorPawn* ResultPawn = Cast<AInstructorPawn>(
			Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot));
		return ResultPawn;
	}

	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}