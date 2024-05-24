#pragma once

#include "CoreMinimal.h"
#include "SlottableComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "SlotComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlotEndOverlapSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotBeginOverlapSignature, USlottableComponent*, SlottableComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotSignature, USlottableComponent*, SlottableComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnslotSignature, USlottableComponent*, SlottableComponent);

UCLASS(Blueprintable, ClassGroup=(VRSimulation),HideCategories=(Materials), meta=(BlueprintSpawnableComponent))
class SIMUL_API USlotComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UCapsuleComponent* CustomRangeComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsEnable;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsRendered;

	UPROPERTY(EditAnywhere)
	bool bUsingCustomRange;

	UPROPERTY(EditAnywhere, meta = (EditCondition="bUsingCustomRange"))
	float CapsuleHalfHeight;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition="bUsingCustomRange"))
	float CapsuleRadius;

	UPROPERTY(BlueprintReadOnly)
	bool bIsSlotted;

	UPROPERTY(EditAnywhere, meta=(EditCondition="bUsingCustomRange"))
	FTransform CustomRangeTransform;

	UPROPERTY(EditAnywhere)
	FLinearColor IdleColor;
	
	UPROPERTY(EditAnywhere)
	FLinearColor OverlappingColor;
	
	UPROPERTY()
	UMaterialInterface* SlotSourceMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* SlotMaterialInstance;
	
	UPROPERTY(BlueprintAssignable)
	FOnSlotBeginOverlapSignature OnSlotBeginOverlap;
	
	UPROPERTY(BlueprintAssignable)
	FOnSlotEndOverlapSignature OnSlotEndOverlap;
	
	UPROPERTY(BlueprintAssignable)
	FOnSlotSignature OnSlot;
	
	UPROPERTY(BlueprintAssignable)
	FOnSlotSignature OnUnslot;
	
public:
	USlotComponent();

#if WITH_EDITOR	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	virtual void DestroyComponent(bool bPromoteChildren) override;

	UFUNCTION(BlueprintCallable)
	void SetSlotEnable(bool bEnabled);
	
	UFUNCTION(BlueprintCallable)
	void SetSlotVisible(bool bNewVisibility);
	
	UFUNCTION()
	void DefaultOnSlot(USlottableComponent* SlottableComponent);

	UFUNCTION()
	void DefaultOnUnslot(USlottableComponent* SlottableComponent);
	
	UFUNCTION()
	void DefaultOnSlotBeginOverlap(USlottableComponent* SlottableComponent);

	UFUNCTION()
	void DefaultOnSlotEndOverlap();
	

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
private:
	void UpdateCustomRangeSettings();
};