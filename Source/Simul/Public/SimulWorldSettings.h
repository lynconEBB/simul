#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "Simulation/SimulationDataTypes.h"
#include "SimulWorldSettings.generated.h"

UCLASS(ClassGroup=(VRSimulation))
class SIMUL_API ASimulWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Character Spawn")
	bool bForcePlayerStartTransformVR;	
	
	UPROPERTY(EditAnywhere, Category="Simulation")
	bool bIsSimulationMap;
	
	UPROPERTY(EditAnywhere, Category="Simulation")
	bool bShowDefaultSimulationWidget;

	UPROPERTY(EditAnywhere, Category="Simulation")
	TArray<FSimulation> AvailableSimulations;

	UPROPERTY(EditAnywhere, Category="Simulation")
	FName DefaultSimulationId;
};
