#pragma once

#include "CoreMinimal.h"
#include "GripScripts/VRGripScriptBase.h"
#include "GS_PreventCollision.generated.h"

class UPrimitiveComponent;

UCLASS(NotBlueprintable, HideCategories = TickSettings ,ClassGroup=(VRSimulation))
class SIMUL_API UGS_PreventCollision : public UVRGripScriptBase
{
	GENERATED_BODY()

public:
	UGS_PreventCollision(const FObjectInitializer& ObjectInitializer);
	virtual void OnGrip_Implementation(UGripMotionControllerComponent* GrippingController, const FBPActorGripInformation& GripInformation) override;
	UFUNCTION()
	void OnLerpFinished(uint8 GripID);
	UFUNCTION()
	void OnLerpFinished_Fallback(const FBPActorGripInformation& GripInformation);
	virtual void OnGripRelease_Implementation(UGripMotionControllerComponent* ReleasingController, const FBPActorGripInformation& GripInformation, bool bWasSocketed) override;
	virtual void Tick(float DeltaTime) override;

private:
	bool IsObjectPenetrating(UPrimitiveComponent* Component);
	
	UPROPERTY()
	TMap<uint8,UPrimitiveComponent*> GripCache;
	TMap<uint8,FCollisionResponseContainer> CollisionCache;
};
