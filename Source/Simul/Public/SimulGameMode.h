#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SimulGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStepClearedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStepMistakeSignature, FText, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQuestClearSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShowHelperUpdateSignature, bool, bNewValue);

class ASimulationManager;
class USimulationDescriptor;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(VRSimulation))
class SIMUL_API ASimulGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadOnly)
	bool bShowHelpMarkers;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FShowHelperUpdateSignature OnShowHelperUpdate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classes")
	TSubclassOf<APawn> VRPawnClass;

public:
	ASimulGameMode();

	FORCEINLINE bool IsShowingHelperMarkers()
	{
		return bShowHelpMarkers;
	}

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	
	UClass* GetDesiredPlayerStartClass(int32 ControllerId);

	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal,
	                                 const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	
protected:
	UFUNCTION(BlueprintCallable)
	void SetShowHelperMarkers(bool bShowHelpers);
};