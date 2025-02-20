#include "Simulation/SimulationSubsystem.h"

#include "SimulWorldSettings.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Simulation/SimulationDataTypes.h"


void USimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	InitGameModeHandle = FGameModeEvents::GameModeInitializedEvent.AddUObject(this, &USimulationSubsystem::OnGameModeInitialized);
}

void USimulationSubsystem::Deinitialize()
{
	FGameModeEvents::GameModeInitializedEvent.Remove(InitGameModeHandle);
}

void USimulationSubsystem::OnGameModeInitialized(AGameModeBase* GameModeBase)
{
	ASimulWorldSettings* WorldSettings = Cast<ASimulWorldSettings>(GetWorld()->GetWorldSettings());
	if (!IsValid(WorldSettings))
		return;
	TArray<FSimulation>* AvailableSimulations = &WorldSettings->AvailableSimulations;
	
	// Check if has simulation option
	FName SimId; 
	if (!UGameplayStatics::HasOption(GameModeBase->OptionsString, TEXT("SimId")))
	{
		SimId = WorldSettings->DefaultSimulationId;
	}
	else
	{
		SimId = FName(UGameplayStatics::ParseOption(GameModeBase->OptionsString, TEXT("SimId")));
	}

	// Get the simulation id from option
	// Find Simulation based on Simulation id
	for (const FSimulation Simulation : *AvailableSimulations)
	{
		if (SimId == Simulation.Id)
		{
			CurrentSimulation = Simulation;
		}
	}
	
	// Check if simulation was found
	if (CurrentSimulation.Id == NAME_None)
	{
		UE_LOG(LogBlueprintUserMessages, Error, TEXT("Could not start simulation subsystem: Simulation not found by ID!"));
		return;
	}

	if (CurrentSimulation.States)
	{
		// Create and init states used by simulation 
		for (FName& StateId : CurrentSimulation.States->GetRowNames())
		{
			FStateDescriptor* StateDescriptor = CurrentSimulation.States->FindRow<FStateDescriptor>(StateId, "State descriptor data table");
			UState* State = NewObject<UState>(this);
			State->Init(StateDescriptor);
			StatesMap.Add(StateId, State);
		}

		// Post init all states
		for (const TPair<FName, UState*>& Pair : StatesMap)
		{
			FStateDescriptor* StateDescriptor = CurrentSimulation.States->FindRow<FStateDescriptor>(Pair.Key, "State descriptor data table");
			Pair.Value->PostInit(StateDescriptor);
		}
	}

	if (CurrentSimulation.Mistakes)
	{
		for (FName& MistakeId : CurrentSimulation.Mistakes->GetRowNames())
		{
			FMistakeDescriptor* MistakeDescriptor = CurrentSimulation.Mistakes->FindRow<FMistakeDescriptor>(MistakeId, "Mistake descriptor data table");
			UMistake* Mistake = NewObject<UMistake>(this);
			Mistake->Init(MistakeId, MistakeDescriptor);
			MistakesMap.Add(MistakeId, Mistake);
		}
	}

	// Load required stream level
	if (CurrentSimulation.MapToLoad.IsValid())
		UGameplayStatics::LoadStreamLevelBySoftObjectPtr(this, CurrentSimulation.MapToLoad, true, true, FLatentActionInfo());
}

UState* USimulationSubsystem::GetState(FDataTableRowHandle State)
{
	return StatesMap.FindRef(State.RowName);
}

TArray<FName> USimulationSubsystem::GetAllStatesId()
{
	TArray<FName> IDs;
	StatesMap.GetKeys(IDs);
	return IDs;
}

UState* USimulationSubsystem::GetStateById(FName StateId)
{
	return StatesMap.FindRef(StateId);
}

TArray<UState*> USimulationSubsystem::GetAllStates()
{
	TArray<UState*> States;
	for (const TPair<FName, UState*>& Pair : StatesMap)
	{
		States.Add(Pair.Value);	
	}
	return States;	
}

FString USimulationSubsystem::GetCurrentSimulationName() const
{
	return CurrentSimulation.Name;
}

bool USimulationSubsystem::IsOnSimulationMap() const
{
	ASimulWorldSettings* WorldSettings = Cast<ASimulWorldSettings>(GetWorld()->GetWorldSettings());
	return WorldSettings && WorldSettings->bIsSimulationMap;
}

FName USimulationSubsystem::GetCurrentSimulationId() const
{
	return CurrentSimulation.Id;
}

FTimespan USimulationSubsystem::GetTimeAsSpan(const UObject* WorldContextObject) 
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	
	int32 Minutes = World->GetTimeSeconds() / 60;
	int32 Seconds = int(World->GetTimeSeconds()) % 60;
	return FTimespan(0, Minutes, Seconds);
}

bool USimulationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;	
	}

	UWorld* World = Cast<UWorld>(Outer);
	check(World);
	return Cast<ASimulWorldSettings>(World->GetWorldSettings()) != nullptr;
}

void USimulationSubsystem::TravelToSimulation(const UObject* WorldContextObject, const TSoftObjectPtr<UWorld> Level,
	FName SimulationID)
{
	FString Options = FString::Printf(TEXT("SimId=%s"), *SimulationID.ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContextObject, Level, true, Options);
}

void USimulationSubsystem::ThrowMistake(FDataTableRowHandle MistakeHandle)
{
	ThrowMistake(MistakeHandle.RowName);
}

void USimulationSubsystem::ThrowMistake(FName MistakeName)
{
	UMistake** MistakePtr = MistakesMap.Find(MistakeName);
	if (MistakePtr != nullptr)
	{
		UMistake* MistakeFound = *MistakePtr;
		if (MistakeFound->CanBeThrown())
		{
			FTimespan TimeElapsed = GetTimeAsSpan(this);
			MistakeOccurrences.Emplace(MistakeName, TimeElapsed);
			OnMistakeOccurred.Broadcast(TimeElapsed, MistakeFound);
		}
	} else
	{
		UE_LOG(LogBlueprintUserMessages, Error, TEXT("Could not found required mistake to throw!"));
	}
}

UMistake* USimulationSubsystem::GetLastMistake()
{
	if (MistakeOccurrences.Num() > 0)
	{
		return *MistakesMap.Find(MistakeOccurrences.Top().Name);
	}
	return nullptr;
}

UMistake* USimulationSubsystem::GetLastMistakeByName(const FName& Name)
{
	for (int i = MistakeOccurrences.Num() - 1; i >= 0; i--)
	{
		if (MistakeOccurrences[i].Name == Name)
			return *MistakesMap.Find(MistakeOccurrences[i].Name);
	}
	return nullptr;
}

UMistake* USimulationSubsystem::GetMistakeByName(const FName& Name)
{
	return MistakesMap.FindRef(Name);
}

TArray<FMistakeOccurrence> USimulationSubsystem::GetAllMistakeOccurrences()
{
	return MistakeOccurrences;
}

bool USimulationSubsystem::UpdateState(FDataTableRowHandle InState, bool bNewActive)
{
	UState* State = GetState(InState);
	if (!IsValid(State) || State->bIsActive == bNewActive)
		return false;

	State->Update(bNewActive);

	bool bAllRequiredStatesActive = true;
	for (const TPair<FName, UState*>& Pair : StatesMap)
	{
		const UState* Current = Pair.Value;
		if (Current->bIsRequired && !Current->bIsActive)
			bAllRequiredStatesActive = false;
	}
	
	if (bAllRequiredStatesActive)
	{
		OnSimulationEnd.Broadcast();		
	}
	
	return true;
}

bool USimulationSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}