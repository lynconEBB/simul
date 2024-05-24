#pragma once

#include "CoreMinimal.h"
#include "Simulation/SimulationDataTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Map.h"
#include "SimulationSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMistakeThrowDelegate, FTimespan, MistakeTime, UMistake*, Mistake );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimulationTimeTickDelegate, FTimespan, Time);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimulationEndDelegate);

struct FUser;

UCLASS(BlueprintType, ClassGroup=(VRSimulation))
class SIMUL_API USimulationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	
	void OnGameModeInitialized(AGameModeBase* GameModeBase);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;
	
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	
	UFUNCTION(BlueprintCallable)
	UState* GetState(FDataTableRowHandle State);
	
	UFUNCTION(BlueprintCallable,BlueprintPure)
	TArray<FName> GetAllStatesId();

	UFUNCTION(BlueprintCallable)
	UState* GetStateById(FName StateId);

	UFUNCTION(BlueprintCallable)
	TArray<UState*> GetAllStates();
	
	UFUNCTION(BlueprintCallable)
	void ThrowMistake(FDataTableRowHandle Mistake);
	
	void ThrowMistake(FName MistakeName);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UMistake* GetLastMistake();
	
	UMistake* GetLastMistakeByName(const FName& Name);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	UMistake* GetMistakeByName(const FName& Name);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FMistakeOccurrence> GetAllMistakeOccurrences();
	
	UFUNCTION(BlueprintCallable)
	UPARAM(DisplayName="WasUpdated") bool UpdateState(FDataTableRowHandle InState, bool bNewActive);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetCurrentSimulationName() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsOnSimulationMap() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FName GetCurrentSimulationId() const;
	
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject", Category="Simulation"))
	static void TravelToSimulation(const UObject* WorldContextObject, const TSoftObjectPtr<UWorld> Level, FName SimulationID);

	UFUNCTION(BlueprintCallable,BlueprintPure, meta=(WorldContext="WorldContextObject", Category="Simulation"))
	static FTimespan GetTimeAsSpan(const UObject* WorldContextObject);
	
	UPROPERTY()
	TArray<FMistakeOccurrence> MistakeOccurrences;

	UPROPERTY(BlueprintAssignable, BlueprintReadOnly,BlueprintCallable)
	FSimulationEndDelegate OnSimulationEnd;
	
protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	
private:
	FDelegateHandle InitGameModeHandle;
	
	FSimulation CurrentSimulation;

	UPROPERTY()
	TMap<FName, UState*> StatesMap;
	UPROPERTY()
	TMap<FName, UMistake*> MistakesMap;

	UPROPERTY(BlueprintAssignable)
	FMistakeThrowDelegate OnMistakeOccurred;
};