#include "CollisionSolvingSubsystem.h"

#include "DrawDebugHelpers.h"
#include "VRExpansionFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CollisionIgnoreSubsystem.h"

void UCollisionSolvingSubsystem::UpdateCollision(FSolvingInfo Info)
{
	if (SolvingInfos.Find(Info) != INDEX_NONE)
		return;

	if (Info.bUseBounds)
	{
		int32 count = 0;
		for (uint8 Response : Info.TargetCollision.EnumArray)
		{
			ECollisionChannel Channel = UCollisionProfile::Get()->ConvertToCollisionChannel(false, count);
			if (Response == ECR_Block && Info.IgnoreChannels.Find(Channel) == INDEX_NONE)
			{
				EObjectTypeQuery ObjectType = UCollisionProfile::Get()->ConvertToObjectType(Channel);
				if (ObjectType != ObjectTypeQuery_MAX)
					Info.ObjectTypes.Add(ObjectType);
			}
			count++;
		}
	}

	if ((Info.bUseBounds && !AreBoundsPenetrating(Info)) || (!Info.bUseBounds && !IsObjectPenetrating(Info)))
	{
		Info.Component->SetCollisionResponseToChannels(Info.TargetCollision);
		return;
	}

	SolvingInfos.Emplace(Info);
}

void UCollisionSolvingSubsystem::UpdateCollision(UPrimitiveComponent* Comp1, UPrimitiveComponent* Comp2)
{
	if (!Comp1->IsOverlappingComponent(Comp2))
	{
		UVRExpansionFunctionLibrary::SetObjectsIgnoreCollision(this, Comp1, NAME_None, false,
			Comp2, NAME_None, false, false);
		return;
	}

	TrackedComponents.Add(TPair<UPrimitiveComponent*, UPrimitiveComponent*>(Comp1, Comp2));
}

void UCollisionSolvingSubsystem::UpdateCollision(AActor* Actor1, AActor* Actor2)
{
	if (!Actor1->IsOverlappingActor(Actor2))
	{
		UVRExpansionFunctionLibrary::SetActorsIgnoreAllCollision(this, Actor1, Actor2, false);
		return;
	}

	TrackedActors.Add(TPair<AActor*, AActor*>(Actor1, Actor2));
}

void UCollisionSolvingSubsystem::Tick(float DeltaTime)
{
	for (const FSolvingInfo& Info : SolvingInfos)
	{
		if ((Info.bUseBounds && !AreBoundsPenetrating(Info)) || (!Info.bUseBounds && !IsObjectPenetrating(Info)))
		{
			Info.Component->SetCollisionResponseToChannels(Info.TargetCollision);
			SolvingInfos.RemoveSingle(Info);
			return;
		}
	}
	for (TPair<UPrimitiveComponent*, UPrimitiveComponent*> Pair : TrackedComponents)
	{
		if (!Pair.Key->IsOverlappingComponent(Pair.Value))
		{
			UVRExpansionFunctionLibrary::SetObjectsIgnoreCollision(this, Pair.Key,
				NAME_None, nullptr, Pair.Value, NAME_None, nullptr, false );	
		}	
	}
	for (TPair<AActor*, AActor*> Pair : TrackedActors)
	{
		if (!Pair.Key->IsOverlappingActor(Pair.Value))
		{
			UVRExpansionFunctionLibrary::SetActorsIgnoreAllCollision(this,Pair.Key, Pair.Value, false);
		}	
	}
}

bool UCollisionSolvingSubsystem::IsObjectPenetrating(const FSolvingInfo& Info)
{
	TArray<UPrimitiveComponent*> OverlappingComps;
	Info.Component->GetOverlappingComponents(OverlappingComps);

	for (UPrimitiveComponent* Comp : OverlappingComps)
	{
		if (Info.IgnoreChannels.Find(Comp->GetCollisionObjectType()) == INDEX_NONE
			&& Comp->GetCollisionResponseToChannel(Info.Component->GetCollisionObjectType()) == ECR_Block
			&& (!Info.bLimitToSimulated || Comp->IsSimulatingPhysics()))
		{
			return true;
		}
	}
	return false;
}

bool UCollisionSolvingSubsystem::AreBoundsPenetrating(const FSolvingInfo& Info)
{
	FBoxSphereBounds BodyBounds = Info.Component->Bounds;
	// DrawDebugBox(GetWorld(),BodyBounds.Origin, BodyBounds.BoxExtent, FColor::Cyan, false, 0.05,0,1);

	TArray<UPrimitiveComponent*> OverlappingComps;
	UKismetSystemLibrary::BoxOverlapComponents(this, BodyBounds.Origin, BodyBounds.BoxExtent, Info.ObjectTypes, nullptr,
	                                           TArray<AActor*>(), OverlappingComps);
	for (UPrimitiveComponent* Comp : OverlappingComps)
	{
		if (Info.IgnoreChannels.Find(Comp->GetCollisionObjectType()) == INDEX_NONE
			&& Comp->GetCollisionResponseToChannel(Info.Component->GetCollisionObjectType()) == ECR_Block
			&& (!Info.bLimitToSimulated || Comp->IsSimulatingPhysics()))
			return true;
	}
	return false;
}

UWorld* UCollisionSolvingSubsystem::GetTickableGameObjectWorld() const
{
	return Super::GetTickableGameObjectWorld();
}

TStatId UCollisionSolvingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCollisionSolvingSubsystem, STATGROUP_Tickables);
}
