#include "TrainingCharacter/HandAnimInstance.h"
#include "TrainingCharacter/GraspingHand.h"

void UHandAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningHand = Cast<AGraspingHand>(GetOwningActor());
}

void UHandAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (OwningHand)
	{
		if (OwningHand->AnimState == EHandAnimState::Snapshot && HandState != EHandAnimState::Snapshot)
		{
			CustomGrabPose = OwningHand->PoseSnapshot;
		}
		
		HandState = OwningHand->AnimState;
		
		if (HandState == EHandAnimState::Procedural)
		{
			FingerCurls = OwningHand->FingersInfo.FingersCurl;
		}
	}
}
