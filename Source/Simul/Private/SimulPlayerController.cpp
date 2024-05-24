#include "SimulPlayerController.h"

#include "SimulWorldSettings.h"
#include "Blueprint/UserWidget.h"
#include "Instructor/InstructorPawn.h"
#include "Instructor/InstructorScreenSubsystem.h"
#include "CustomComponents/CustomSceneCaptureComponent.h"
#include "Engine/LocalPlayer.h"


void ASimulPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (GetLocalPlayer()->GetControllerId() == 1)
	{
		InputComponent->BindAction("ToggleCursor", IE_Released, this, &ASimulPlayerController::ToggleCursor );
	}
}


void ASimulPlayerController::ToggleCursor()
{
	APlayerController* FirstPlayerController = GetWorld()->GetFirstPlayerController();

	if (!bPossessingPawn)
	{
		return;
	}
	
	if (FirstPlayerController->bShowMouseCursor)
	{
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(true);
		FirstPlayerController->SetInputMode(InputMode);
	} else
	{
		FInputModeGameAndUI InputMode;
		
		InputMode.SetWidgetToFocus(nullptr);
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		FirstPlayerController->SetInputMode(InputMode);
	}
	
	FirstPlayerController->SetShowMouseCursor(!FirstPlayerController->bShowMouseCursor);
}

void ASimulPlayerController::OnPossess(APawn* InPawn)
{
	if (!HasActorBegunPlay() && GetLocalPlayer()->GetControllerId() == 1)
	{
		return;
	}

	Super::OnPossess(InPawn);
	bPossessingPawn = true;

	if (GetLocalPlayer()->GetControllerId() == 1)
	{
		AInstructorPawn* InstructorPawn = Cast<AInstructorPawn>(InPawn);
		UInstructorScreenSubsystem* InstructorScreenSubsystem = GetGameInstance()->GetSubsystem<UInstructorScreenSubsystem>();
		InstructorScreenSubsystem->SetViewTarget(false, InstructorPawn->SceneCaptureComponent);
	}
}

void ASimulPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
	bPossessingPawn = false;

	if (GetLocalPlayer()->GetControllerId() == 1)
	{
		UInstructorScreenSubsystem* InstructorScreenSubsystem = GetGameInstance()->GetSubsystem<UInstructorScreenSubsystem>();
		InstructorScreenSubsystem->SetViewTarget(true, nullptr);
		GetWorld()->GetFirstPlayerController()->SetShowMouseCursor(true);	
	}
}


void ASimulPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (GetLocalPlayer()->GetControllerId() == 0)
		return;
	
	UInstructorScreenSubsystem* InstructorScreenSubsystem = GetGameInstance()->GetSubsystem<UInstructorScreenSubsystem>();
	InstructorScreenSubsystem->SetViewTarget(true, nullptr);
	GetWorld()->GetFirstPlayerController()->SetShowMouseCursor(true);

	ASimulWorldSettings* WorldSettings = Cast<ASimulWorldSettings>(GetWorldSettings());
	if (WorldSettings == nullptr)
	{
		UE_LOG(LogBlueprintUserMessages, Error, TEXT("Simul Player controller cant find data needed in World settings!"));
		return;
	}
	if (DefaultSimulationWidgetClass == nullptr)
	{
		UE_LOG(LogBlueprintUserMessages, Error, TEXT("Default simulation Widget not provided!"));
		return;
	}
	
	if (WorldSettings->bShowDefaultSimulationWidget)
	{
		SimulationWidget = CreateWidget(this, DefaultSimulationWidgetClass);
		InstructorScreenSubsystem->AddToScreen(SimulationWidget);
	}
}

// Do not apply HMD rotation to Player 1
void ASimulPlayerController::UpdateRotation(float DeltaTime)
{
	if (GetLocalPlayer()->GetControllerId() == 1)
	{
		FRotator DeltaRot(RotationInput);
		FRotator ViewRotation = GetControlRotation();

		if (PlayerCameraManager)
			PlayerCameraManager->ProcessViewRotation(DeltaTime, ViewRotation, DeltaRot);

		SetControlRotation(ViewRotation);
		
		APawn* const P = GetPawnOrSpectator();
		if (P)
			P->FaceRotation(ViewRotation, DeltaTime);
	}
	else
	{
		Super::UpdateRotation(DeltaTime);
	}
}

bool ASimulPlayerController::CanRestartPlayer()
{
	if (GetLocalPlayer()->GetControllerId() == 0)
		return Super::CanRestartPlayer();
		
	ASimulWorldSettings* WorldSettings = Cast<ASimulWorldSettings>(GetWorldSettings());
	if (WorldSettings == nullptr)
	{
		UE_LOG(LogBlueprintUserMessages, Error, TEXT("Simul Player controller cant find data needed in World settings!"));
		return Super::CanRestartPlayer();
	}
	
	return Super::CanRestartPlayer() && WorldSettings->bIsSimulationMap;
}
