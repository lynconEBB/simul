#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SimulPlayerController.generated.h"

UCLASS(ClassGroup=(VRSimulation))
class SIMUL_API ASimulPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSubclassOf<UUserWidget> DefaultSimulationWidgetClass;

private:
	UPROPERTY()
	UUserWidget* SimulationWidget;
	
	bool bPossessingPawn;

protected:	
	virtual void BeginPlay() override;
	
public:
	virtual void UpdateRotation(float DeltaTime) override;
	
	virtual bool CanRestartPlayer() override;
	
	UFUNCTION()
	void ToggleCursor();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
	virtual void SetupInputComponent() override;
};
