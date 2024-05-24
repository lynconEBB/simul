
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "SimulationDataTypes.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChangeStateDelegate, bool, bNewState, UState*, CallingState);

USTRUCT(BlueprintType)
struct FPrerequisite
{
	GENERATED_BODY()
	
public:
	FPrerequisite()
		: bRequiredActive(false)
	{ }

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDataTableRowHandle State;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRequiredActive;
};

UENUM(BlueprintType)
enum class ESeverity : uint8
{
	Error,
	Warning,
	Info
};

USTRUCT(BlueprintType)
struct FMistakeOccurrence
{
	GENERATED_BODY()
	
public:
	FMistakeOccurrence(): Name(NAME_None) {}
	FMistakeOccurrence(FName Name, FTimespan Time) : Name(Name), Time(Time) {}
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FName Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FTimespan Time;
};

USTRUCT(BlueprintType)
struct FMistakeDescriptor : public FTableRowBase
{
	GENERATED_BODY()
public:
	FMistakeDescriptor() :
		Severity(ESeverity::Error),
		bSequentiallyFired(false),
		bSingleFired(false),
		GroupName(NAME_None){}	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	ESeverity Severity;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	bool bSequentiallyFired;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	bool bSingleFired;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	FName GroupName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	TArray<FPrerequisite> Prerequisites;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	TArray<FDataTableRowHandle> BoundStates;
};


UCLASS(BlueprintType, Blueprintable)
class UMistake : public UObject
{
	GENERATED_BODY()

public:
	void Init(const FName& InName, FMistakeDescriptor* Descriptor);
	bool CanBeThrown(); 
	
private:
	UFUNCTION()
	void OnBoundStateChanged(bool bNewState, UState* CallingState);
	
public:
	TWeakObjectPtr<class USimulationSubsystem> Subsystem;
	
	UPROPERTY(BlueprintReadOnly, Category="Simulation")
	FName Name;
	
	UPROPERTY(BlueprintReadOnly, Category="Simulation")
	FName GroupName;
	
	UPROPERTY(BlueprintReadOnly, Category="Simulation")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category="Simulation")
	ESeverity Severity;
	
	UPROPERTY(BlueprintReadOnly, Category="Simulation")
	bool bSequentiallyFired;
	
	UPROPERTY(BlueprintReadOnly, Category="Simulation")
	bool bSingleFired;

	UPROPERTY(BlueprintReadOnly, Category="Simulation")
	TMap<UState*, bool> Prerequisites;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<UState*> BoundStates;
};

USTRUCT(BlueprintType)
struct FStateDescriptor : public FTableRowBase
{
	GENERATED_BODY()
	FStateDescriptor() : bIsAux(false),bIsRequired(true) {}
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	bool bIsAux;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	bool bIsRequired;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	TArray<FName> PreviousStates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation")
	TArray<FName> NextStates;
};


UCLASS(BlueprintType)
class SIMUL_API UState : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FName Name;

	UPROPERTY(BlueprintReadOnly)
	bool bIsActive;

	UPROPERTY(BlueprintReadOnly)
	bool bIsAux;
	
	UPROPERTY(BlueprintReadWrite)
	bool bIsRequired;

	UPROPERTY(BlueprintAssignable)
	FOnChangeStateDelegate OnChange;

	TWeakObjectPtr<class USimulationSubsystem> Subsystem;

	TArray<TWeakObjectPtr<UState>> PreStates;
	
	TArray<TWeakObjectPtr<UState>> PostStates;
	
public:
	void Init(FStateDescriptor* StateDescriptor);
	
	void PostInit(FStateDescriptor* StateDescriptor);
	
	void Update(bool bNewActive);
	
	UFUNCTION()
	void OnDependentStateChange(bool bNewState, UState* CallingState);
};


USTRUCT(BlueprintType)
struct FSimulation
{
	GENERATED_BODY()
	
	FSimulation() : States(nullptr), Mistakes(nullptr) {}
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Simulation")
	FName Id;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Simulation")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Simulation")
	TSoftObjectPtr<UWorld> MapToLoad;

	UPROPERTY(EditAnywhere, Category="Simulation", meta=(RequiredAssetDataTags="RowStructure=StateDescriptor"))
	UDataTable* States;
	
	UPROPERTY(EditAnywhere, Category="Simulation", meta=(RequiredAssetDataTags="RowStructure=MistakeDescriptor"))
	UDataTable* Mistakes;

	bool operator== (FName OtherId) const
	{
		return this->Id == OtherId;	
	}
};