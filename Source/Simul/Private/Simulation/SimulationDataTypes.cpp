#include "Simulation/SimulationDataTypes.h"

#include "Simulation/SimulationSubsystem.h"


void UMistake::Init(const FName& InName, FMistakeDescriptor* Descriptor)
{
	Name = InName;
	Subsystem = GetWorld()->GetSubsystem<USimulationSubsystem>();
	Description = Descriptor->Description;
	Severity = Descriptor->Severity;
	bSequentiallyFired = Descriptor->bSequentiallyFired;
	bSingleFired = Descriptor->bSingleFired;
	GroupName = Descriptor->GroupName;
	
	for (const FPrerequisite& Prerequisite : Descriptor->Prerequisites)
	{
		UState* State = Subsystem->GetState(Prerequisite.State);
		Prerequisites.Add(State, Prerequisite.bRequiredActive);
	}
	
	for (FDataTableRowHandle StateRow : Descriptor->BoundStates)
	{
		UState* State = Subsystem->GetState(StateRow);
		BoundStates.Add(State);
		State->OnChange.AddDynamic(this, &UMistake::OnBoundStateChanged);
	}
}

bool UMistake::CanBeThrown()
{
	UMistake* LastMistake = Subsystem->GetLastMistake();
	if (!bSequentiallyFired && LastMistake != nullptr)
	{
		if (LastMistake->GroupName == GroupName && LastMistake->GroupName != NAME_None)
		{
			return false;
		}
		if (LastMistake->Name == Name)
		{
			return false;
		}
	}
	
	if (bSingleFired && Subsystem->GetLastMistakeByName(Name) != nullptr)
	{
		return false;	
	}
	
	if (BoundStates.Num() > 0)
	{
		for (TPair<UState*, bool> Prerequisite : Prerequisites)
		{
			if (Prerequisite.Key->bIsActive == Prerequisite.Value)
				return true;	
		}
		return false;
	}
	
	for (TPair<UState*, bool> Prerequisite : Prerequisites)
	{
		if (Prerequisite.Key->bIsActive != Prerequisite.Value)
			return false;	
	}
	
	return true;
}

void UMistake::OnBoundStateChanged(bool bNewState, UState* CallingState)
{
	if (bNewState == true)
		Subsystem->ThrowMistake(Name);
}

void UState::Init(FStateDescriptor* StateDescriptor)
{
	Subsystem = GetWorld()->GetSubsystem<USimulationSubsystem>();
	Name = StateDescriptor->Name;
	bIsAux = StateDescriptor->bIsAux;
	bIsRequired = StateDescriptor->bIsRequired;
	bIsActive = false;
}

void UState::Update(bool bNewActive)
{
	// Early return if we are already in desiredState
	if (bIsActive == bNewActive)
		return;

	// If all ok, update
	bIsActive = bNewActive;
	OnChange.Broadcast(bNewActive, this);
}

void UState::PostInit(FStateDescriptor* StateDescriptor)
{
	for (const FName& StateId : StateDescriptor->PreviousStates)
	{
		UState* State = Subsystem->GetStateById(StateId);
		PreStates.Add(State);
		State->OnChange.AddDynamic(this, &UState::OnDependentStateChange);
	}
	
	for (const FName& StateId : StateDescriptor->NextStates)
	{
		UState* State = Subsystem->GetStateById(StateId);
		PostStates.Add(State);
		State->OnChange.AddDynamic(this, &UState::OnDependentStateChange);
	}
}

void UState::OnDependentStateChange(bool bNewState, UState* CallingState)
{
	bool bShouldBeActive = true;
	for (TWeakObjectPtr<UState> State : PreStates)
	{
		if (State.Get()->bIsActive != true)
			bShouldBeActive = false;
	}
	for (TWeakObjectPtr<UState> State : PostStates)
	{
		if (State.Get()->bIsActive != false)
			bShouldBeActive = false;
	}
	
	if (bIsActive != bShouldBeActive)
	{
		bIsActive = bShouldBeActive;
		OnChange.Broadcast(bShouldBeActive, this);
	}
}
