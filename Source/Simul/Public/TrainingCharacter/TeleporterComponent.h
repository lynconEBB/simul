#pragma once

#include "CoreMinimal.h"
#include "MotionControllerComponent.h"
#include "TeleporterComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class AVRCharacter;

UCLASS(ClassGroup=(VRSimulation), meta=(BlueprintSpawnableComponent))
class SIMUL_API UTeleporterComponent : public UMotionControllerComponent
{
	GENERATED_BODY()

public:
	UTeleporterComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	UFUNCTION(BlueprintCallable)
	void AddActorToIgnoreTrace(AActor* Actor);
	UFUNCTION(BlueprintCallable)
	void SetTraceActive(bool bInActive);
	
	bool GetTeleporterTargetPosition(FVector&  OutPosition);
	
	FORCEINLINE bool IsTraceActive() { return bTraceActive; }
	
protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY()
	UNiagaraComponent* TraceEffectComponent;
	UPROPERTY()
	UNiagaraComponent* RingEffectComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="View")
	UNiagaraSystem* TraceEffectSystem;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="View")
	UNiagaraSystem* RingEffectSystem;;
	
	UPROPERTY(BlueprintReadOnly)
	bool bHasValidTeleportLocation;
	UPROPERTY(BlueprintReadOnly)
	bool bTraceDebug;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LaunchSpeedMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ProjectileRadius;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NavZOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ValidColor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor InvalidColor;
	UPROPERTY(BlueprintReadOnly,Category=Internal)
	bool bIsTeleporting;
	
	
private:
	UPROPERTY()
	AVRCharacter* CharacterOwner;
	UPROPERTY()
	TArray<AActor*> ActorsToIgnoreDuringTrace;
	bool bTraceActive;
	FVector Destination;	
};