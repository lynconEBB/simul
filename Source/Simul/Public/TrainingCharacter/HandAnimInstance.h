#pragma once

#include "CoreMinimal.h"
#include "GraspingHand.h"
#include "Animation/AnimInstance.h"
#include "HandAnimInstance.generated.h"

class AGraspingHand;

UCLASS( ClassGroup=(VRSimulation))
class SIMUL_API UHandAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly)
	AGraspingHand* OwningHand;
	UPROPERTY(BlueprintReadOnly)
	FPoseSnapshot CustomGrabPose;
	UPROPERTY(BlueprintReadOnly)
	TMap<TEnumAsByte<EFingerPart>, float> FingerCurls;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EHandAnimState HandState;
};
