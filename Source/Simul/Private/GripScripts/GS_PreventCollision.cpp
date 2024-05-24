#include "GripScripts/GS_PreventCollision.h"

#include "TrainingCharacter/GraspingHand.h"
#include "GripMotionControllerComponent.h"
#include "TrainingCharacter/TrainingCharacter.h"

UGS_PreventCollision::UGS_PreventCollision(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsActive = false;
	bCanEverTick = true;
	WorldTransformOverrideType = EGSTransformOverrideType::None;
}

void UGS_PreventCollision::OnLerpFinished(uint8 GripID)
{
	UPrimitiveComponent* GrippedComponent = *GripCache.Find(GripID);
	
	if (IsObjectPenetrating(GrippedComponent))
	{
		SetTickEnabled(true);
	} else
	{
		GrippedComponent->SetCollisionResponseToChannels(*CollisionCache.Find(GripID));
	}
}

void UGS_PreventCollision::OnLerpFinished_Fallback(const FBPActorGripInformation& GripInformation)
{
	OnLerpFinished(GripInformation.GripID);
}

void UGS_PreventCollision::OnGrip_Implementation(UGripMotionControllerComponent* GrippingController,
                                                 const FBPActorGripInformation& GripInformation)
{
	if (GripInformation.GripCollisionType == EGripCollisionType::ManipulationGrip || GripInformation.GripCollisionType == EGripCollisionType::CustomGrip)
		return;


	UPrimitiveComponent* GrippedComponent = GripInformation.GripTargetType == EGripTargetType::ComponentGrip
		                                        ? GripInformation.GetGrippedComponent()
		                                        : Cast<UPrimitiveComponent>(GripInformation.GetGrippedActor()->GetRootComponent());
	
	GripCache.Add(GripInformation.GripID, GrippedComponent);
	CollisionCache.Add(GripInformation.GripID, GrippedComponent->GetCollisionResponseToChannels());
	
	GrippedComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
	
	ATrainingCharacter* Character = Cast<ATrainingCharacter>(GrippingController->GetOwner());
	if (Character)
	{
		EControllerHand HandType;
		GrippingController->GetHandType(HandType);
		AGraspingHand* GraspingHand = Character->GetHandByLaterality(HandType);
		GraspingHand->OnHandAttachToObject.AddDynamic(this, &UGS_PreventCollision::OnLerpFinished);
	} else
	{
		GrippingController->OnLerpToHandFinished.AddDynamic(this, &UGS_PreventCollision::OnLerpFinished_Fallback);
	} 
}

void UGS_PreventCollision::OnGripRelease_Implementation(UGripMotionControllerComponent* ReleasingController,
	const FBPActorGripInformation& GripInformation, bool bWasSocketed)
{
	Super::OnGripRelease_Implementation(ReleasingController, GripInformation, bWasSocketed);
	if (GripInformation.GripCollisionType == EGripCollisionType::ManipulationGrip || GripInformation.GripCollisionType == EGripCollisionType::CustomGrip)
		return;
	
	UPrimitiveComponent* GrippedComponent = *GripCache.Find(GripInformation.GripID);
	GrippedComponent->SetCollisionResponseToChannels(*CollisionCache.Find(GripInformation.GripID));
	
	CollisionCache.Remove(GripInformation.GripID);
	GripCache.Remove(GripInformation.GripID);
	
	ATrainingCharacter* Character = Cast<ATrainingCharacter>(ReleasingController->GetOwner());
	if (Character)
	{
		EControllerHand HandType;
		ReleasingController->GetHandType(HandType);
		AGraspingHand* GraspingHand = Character->GetHandByLaterality(HandType);
		GraspingHand->OnHandAttachToObject.RemoveDynamic(this, &UGS_PreventCollision::OnLerpFinished);
	} else
	{
		ReleasingController->OnLerpToHandFinished.RemoveDynamic(this, &UGS_PreventCollision::OnLerpFinished_Fallback);
	} 
}

void UGS_PreventCollision::Tick(float DeltaTime)
{
	bool bShouldDisableTick = true;
	for (const TPair<uint8, UPrimitiveComponent*>& Pair : GripCache)
	{
		if (!IsObjectPenetrating(Pair.Value))
		{
			FCollisionResponseContainer OriginalReponse = *CollisionCache.Find(Pair.Key);
			Pair.Value->SetCollisionResponseToChannels(OriginalReponse);
		} else
		{
			bShouldDisableTick = false;		
		}
	}

	if (bShouldDisableTick)
		SetTickEnabled(false);
}

bool UGS_PreventCollision::IsObjectPenetrating(UPrimitiveComponent* Component)
{
	FBoxSphereBounds BodyBounds = Component->Bounds;
	// DrawDebugBox(GetWorld(), BodyBounds.Origin, BodyBounds.BoxExtent, FColor::Emerald, false, 0.05, 0, 1);
	
	TArray<UPrimitiveComponent*> OverlappingComps;
	Component->GetOverlappingComponents(OverlappingComps);	
	for (UPrimitiveComponent* Comp : OverlappingComps)
	{
		if (Comp->GetCollisionObjectType() != ECC_Pawn && Comp->IsSimulatingPhysics() && Comp->GetCollisionResponseToChannel(Component->GetCollisionObjectType()) == ECollisionResponse::ECR_Block)
		{
			FBoxSphereBounds BoxSphereBounds = Comp->Bounds;
			// DrawDebugBox(GetWorld(), BoxSphereBounds.Origin, BoxSphereBounds.BoxExtent, FColor::Magenta, false, 0.05, 0, 1);
			
			return true;
		}
	}
	return false;
}

