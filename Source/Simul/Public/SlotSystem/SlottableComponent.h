#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Grippables/GrippableStaticMeshComponent.h"
#include "SlottableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotedSignature, FTransform, WorldTransform, class USlotComponent*,
                                             SlotComponent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnslotedSignature,class USlotComponent*, SlotComponent);

struct FControlledDelegates
{
	FVROnDropSignature* OnControlledDrop;

	FVROnGripSignature* OnControlledGrip;

	FComponentBeginOverlapSignature* OnControlledOverlap;

	FComponentEndOverlapSignature* OnControlledEndOverlap;
};

UCLASS(ClassGroup=(VRSimulation), meta=(BlueprintSpawnableComponent))
class SIMUL_API USlottableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	USlotComponent* SlotComponent;


	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UObject> GrippableObject;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UPrimitiveComponent> ControlledComponent;

	UPROPERTY(EditAnywhere)
	FName SlotTag;

	UPROPERTY(BlueprintReadOnly)
	bool bAlreadySlotted;
	
	bool bUsingDriver;

	UPROPERTY(BlueprintAssignable)
	FOnSlotedSignature OnSlotted;
	
	UPROPERTY(BlueprintAssignable)
	FOnUnslotedSignature OnUnslotted;
	
	FControlledDelegates Delegates;
	
protected:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	bool bUseRigidBodyDriver;

private:
	TMap<FName, FTransform> ShapesMap;

public:
	USlottableComponent();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleControlledComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                                           const FHitResult& SweepResult);
	UFUNCTION()
	void HandleControlledComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UFUNCTION()
	void HandleControlledComponentDrop(UGripMotionControllerComponent* GrippingController,
	                                   const FBPActorGripInformation& GripInformation,
	                                   bool bWasSocketed);
	UFUNCTION()
	void HandleControlledComponentGrip(UGripMotionControllerComponent* GripMotionControllerComponent,
	                                   const FBPActorGripInformation& FbpActorGripInformation);
	
	UFUNCTION(BlueprintCallable)
	void DoSlot();
	USlotComponent* FindOverlappingSlot(UPrimitiveComponent* OtherComp);

	bool SetupDriver();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
