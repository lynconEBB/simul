#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SimulGameInstance.generated.h"

UCLASS(BlueprintType, Blueprintable, ClassGroup=(VRSimulation))
class SIMUL_API USimulGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual ULocalPlayer* CreateInitialPlayer(FString& OutError) override;
	virtual void Init() override;

#if WITH_EDITOR
	virtual FGameInstancePIEResult StartPlayInEditorGameInstance(ULocalPlayer* LocalPlayer,
		const FGameInstancePIEParameters& Params) override;
#endif
};
