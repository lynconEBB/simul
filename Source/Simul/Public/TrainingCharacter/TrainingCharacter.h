#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "Components/SphereComponent.h"
#include "TrainingCharacter.generated.h"

class ASimulWorldSettings;
DECLARE_DELEGATE_OneParam(FHandActionDelegate, const EControllerHand);
DECLARE_DELEGATE_TwoParams(FHandEnablingDelegate, const EControllerHand, bool bEnabled);

class UTeleporterComponent;
class AGraspingHand;

UCLASS(BlueprintType, Blueprintable, ClassGroup=(VRSimulation))
class SIMUL_API ATrainingCharacter : public AVRCharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USphereComponent* LeftControllerRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USphereComponent* RightControllerRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UTeleporterComponent* LeftTeleporter;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UTeleporterComponent* RightTeleporter;

	UPROPERTY(EditDefaultsOnly, Category="Hands")
	TSubclassOf<AGraspingHand> HandClass;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Hands")
	FTransform RightHandOffset;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Hands")
	FTransform LeftHandOffset;
	UPROPERTY(BlueprintReadOnly)
	AGraspingHand* LeftHand;
	UPROPERTY(BlueprintReadOnly)
	AGraspingHand* RightHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float SnapTurnMultiplier;
private:
	bool bIsTeleporting;
	
	UPROPERTY()
	ASimulWorldSettings* WorldSettings;

public:
	ATrainingCharacter();
	virtual void Tick(float DeltaSeconds) override;
	
	UFUNCTION()
	void TryGrab(const EControllerHand Hand);
	UFUNCTION()
	void TryDrop(EControllerHand ControllerHand);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION()
	void HandleTrackPadClick(const EControllerHand Hand);
	UFUNCTION()
	void HandleTrackpadRelease(EControllerHand ControllerHand);
	
	UFUNCTION()
	void SnapTurn(float SnapDirection);
	UFUNCTION(BlueprintCallable)
	void TeleportTo(FVector TargetLocation, FRotator TargetRotation);
	virtual void PossessedBy(AController* NewController) override;
	
	UFUNCTION()
	void SetPointing(EControllerHand ControllerHand, bool bArg);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	AGraspingHand* GetHandByLaterality(const EControllerHand& Laterality) const;
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UGripMotionControllerComponent* GetOppositeController(UGripMotionControllerComponent* InController) const; 
protected:
	virtual void BeginPlay() override;
	
};
