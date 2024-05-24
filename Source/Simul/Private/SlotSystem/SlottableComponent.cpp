#include "SlotSystem/SlottableComponent.h"

#include "TrainingCharacter/GraspingHand.h"
#include "TrainingCharacter/TrainingCharacter.h"
#include "SlotSystem/SlotComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Grippables/GrippableStaticMeshActor.h"


USlottableComponent::USlottableComponent():
	SlotComponent(nullptr), GrippableObject(nullptr), ControlledComponent(nullptr),
	SlotTag(NAME_None), bAlreadySlotted(false), bUsingDriver(false)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USlottableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->Tags.Contains(SlotTag) && GetOwner()->IsA(AGrippableStaticMeshActor::StaticClass()))
	{
		AGrippableStaticMeshActor* GrippableActor = Cast<AGrippableStaticMeshActor>(GetOwner());
		GrippableObject = GrippableActor;

		UPrimitiveComponent* Component = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
		ControlledComponent = Component;

		Delegates.OnControlledDrop = &GrippableActor->OnDropped;
		Delegates.OnControlledOverlap = &Component->OnComponentBeginOverlap;
		Delegates.OnControlledEndOverlap = &Component->OnComponentEndOverlap;
		Delegates.OnControlledGrip = &GrippableActor->OnGripped;
		Delegates.OnControlledOverlap->AddDynamic(this, &USlottableComponent::HandleControlledComponentBeginOverlap);

		AddTickPrerequisiteComponent(ControlledComponent.Get());
		if (bUseRigidBodyDriver)
			bUsingDriver = SetupDriver();

		return;
	}

	TArray<UActorComponent*> ValidComponents = GetOwner()->GetComponentsByTag(
		UGrippableStaticMeshComponent::StaticClass(), SlotTag);
	if (ValidComponents.Num() > 0)
	{
		UGrippableStaticMeshComponent* GrippableComponent = Cast<UGrippableStaticMeshComponent>(ValidComponents[0]);
		GrippableObject = GrippableComponent;
		ControlledComponent = GrippableComponent;

		Delegates.OnControlledDrop = &GrippableComponent->OnDropped;
		Delegates.OnControlledOverlap = &GrippableComponent->OnComponentBeginOverlap;
		Delegates.OnControlledEndOverlap = &GrippableComponent->OnComponentEndOverlap;
		Delegates.OnControlledGrip = &GrippableComponent->OnGripped;
		Delegates.OnControlledOverlap->AddDynamic(this, &USlottableComponent::HandleControlledComponentBeginOverlap);

		AddTickPrerequisiteComponent(ControlledComponent.Get());
		if (bUseRigidBodyDriver)
			bUsingDriver = SetupDriver();

		return;
	}

	UE_LOG(LogBlueprintUserMessages, Error, TEXT("Could not find tag %s in actor or components to be slottable"),
	       *SlotTag.ToString());
}

USlotComponent* USlottableComponent::FindOverlappingSlot(UPrimitiveComponent* OtherComp)
{
	// Verifica se o componente controlado esta sobrepondo um Slot ou um Range de Slot com Tag Valida
	if (OtherComp->GetClass() == UCapsuleComponent::StaticClass() && OtherComp->GetAttachParent() &&
		OtherComp->GetAttachParent()->GetClass() == USlotComponent::StaticClass() &&
		OtherComp->GetAttachParent()->ComponentTags.Contains(SlotTag))
	{
		return Cast<USlotComponent>(OtherComp->GetAttachParent());
	}

	if (OtherComp->GetClass() == USlotComponent::StaticClass() && OtherComp->ComponentTags.Contains(SlotTag))
	{
		return Cast<USlotComponent>(OtherComp);
	}

	return nullptr;
}


void USlottableComponent::HandleControlledComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                                                bool bFromSweep, const FHitResult& SweepResult)
{
	if (bAlreadySlotted)
		return;
	USlotComponent* OverlapedSlot = FindOverlappingSlot(OtherComp);
	if (OverlapedSlot == nullptr)
		return;

	bool bHadSlotBefore = SlotComponent != nullptr;
	if (bHadSlotBefore && SlotComponent != OverlapedSlot)
		SlotComponent->OnSlotEndOverlap.Broadcast();
	SlotComponent = OverlapedSlot;

	TArray<FBPGripPair> HoldingControllers;
	bool bIsHeld;
	IVRGripInterface::Execute_IsHeld(GrippableObject.Get(), HoldingControllers, bIsHeld);

	if (bIsHeld)
	{
		//Delegates.OnControlledOverlap->RemoveDynamic(this, &USlottableComponent::HandleControlledComponentBeginOverlap);
		if (!bHadSlotBefore)
		{
			Delegates.OnControlledEndOverlap->AddDynamic(
				this, &USlottableComponent::HandleControlledComponentEndOverlap);
			Delegates.OnControlledDrop->AddDynamic(this, &USlottableComponent::HandleControlledComponentDrop);
		}

		SlotComponent->OnSlotBeginOverlap.Broadcast(this);
	}
	else
	{
		DoSlot();
	}
}

void USlottableComponent::HandleControlledComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherComp == SlotComponent || OtherComp->GetClass() == UCapsuleComponent::StaticClass()
		&& OtherComp->GetAttachParent() && OtherComp->GetAttachParent() == SlotComponent)
	{
		Delegates.OnControlledEndOverlap->
		          RemoveDynamic(this, &USlottableComponent::HandleControlledComponentEndOverlap);
		Delegates.OnControlledDrop->RemoveDynamic(this, &USlottableComponent::HandleControlledComponentDrop);

		SlotComponent->OnSlotEndOverlap.Broadcast();
		SlotComponent = nullptr;
	}
}

void USlottableComponent::HandleControlledComponentDrop(UGripMotionControllerComponent* GrippingController,
                                                        const FBPActorGripInformation& GripInformation,
                                                        bool bWasSocketed)
{
	ATrainingCharacter* TrainingCharacter = Cast<ATrainingCharacter>(GrippingController->GetOwner());
	// Make sure to detach hand before sloting
	if (TrainingCharacter)
	{
		EControllerHand HandType;
		GrippingController->GetHandType(HandType);
		AGraspingHand* GraspingHand = TrainingCharacter->GetHandByLaterality(HandType);
		GraspingHand->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
	}

	DoSlot();
}

void ForceComponentUnWeld(UPrimitiveComponent* Component)
{
	FBodyInstance* NewRootBI = Component->GetBodyInstance(NAME_None, false);
	TWeakObjectPtr<UPrimitiveComponent> RootComponent = Component->GetBodyInstance()->OwnerComponent;

	if (RootComponent.IsValid() && RootComponent != Component)
	{
		if (FBodyInstance* RootBI = RootComponent->GetBodyInstance(NAME_None, false))
		{
			bool bRootIsBeingDeleted = RootComponent->IsPendingKillOrUnreachable();
			const FBodyInstance* PrevWeldParent = NewRootBI->WeldParent;
			RootBI->UnWeld(NewRootBI);

			FPlatformAtomics::InterlockedExchangePtr((void**)&NewRootBI->WeldParent, nullptr);

			bool bHasBodySetup = Component->GetBodySetup() != nullptr;

			//if BodyInstance hasn't already been created we need to initialize it
			if (!bRootIsBeingDeleted && bHasBodySetup && NewRootBI->IsValidBodyInstance() == false)
			{
				bool bPrevAutoWeld = NewRootBI->bAutoWeld;
				NewRootBI->bAutoWeld = false;
				NewRootBI->InitBody(Component->GetBodySetup(), Component->GetComponentToWorld(), Component,
				                    Component->GetWorld()->GetPhysicsScene());
				NewRootBI->bAutoWeld = bPrevAutoWeld;
			}

			if (PrevWeldParent == nullptr)
			//our parent is kinematic so no need to do any unwelding/rewelding of children
			{
				return;
			}

			//now weld its children to it
			TArray<FBodyInstance*> ChildrenBodies;
			TArray<FName> ChildrenLabels;
			Component->GetWeldedBodies(ChildrenBodies, ChildrenLabels);

			for (int32 ChildIdx = 0; ChildIdx < ChildrenBodies.Num(); ++ChildIdx)
			{
				FBodyInstance* ChildBI = ChildrenBodies[ChildIdx];
				checkSlow(ChildBI);
				if (ChildBI != NewRootBI)
				{
					if (!bRootIsBeingDeleted)
					{
						RootBI->UnWeld(ChildBI);
					}

					//At this point, NewRootBI must be kinematic because it's being unwelded.
					FPlatformAtomics::InterlockedExchangePtr((void**)&ChildBI->WeldParent, nullptr);
					//null because we are currently kinematic
				}
			}

			//If the new root body is simulating, we need to apply the weld on the children
			if (!bRootIsBeingDeleted && NewRootBI->IsInstanceSimulatingPhysics())
			{
				NewRootBI->ApplyWeldOnChildren();
			}
		}
	}
}

void USlottableComponent::HandleControlledComponentGrip(UGripMotionControllerComponent* GripMotionControllerComponent,
                                                        const FBPActorGripInformation& FbpActorGripInformation)
{
	if (bAlreadySlotted)
	{
		Delegates.OnControlledGrip->RemoveDynamic(this, &USlottableComponent::HandleControlledComponentGrip);
		Delegates.OnControlledOverlap->AddDynamic(
			this, &USlottableComponent::USlottableComponent::HandleControlledComponentBeginOverlap);
		Delegates.OnControlledEndOverlap->AddDynamic(this, &USlottableComponent::HandleControlledComponentEndOverlap);
		Delegates.OnControlledDrop->AddDynamic(this, &USlottableComponent::HandleControlledComponentDrop);

		bAlreadySlotted = false;

		ForceComponentUnWeld(ControlledComponent.Get());
		ControlledComponent->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));

		OnUnslotted.Broadcast(SlotComponent);
		if (SlotComponent)
			SlotComponent->DefaultOnUnslot(this);
	}
}

void USlottableComponent::DoSlot()
{
	if (bAlreadySlotted)
		return;

	Delegates.OnControlledOverlap->RemoveDynamic(this, &USlottableComponent::HandleControlledComponentBeginOverlap);
	Delegates.OnControlledEndOverlap->RemoveDynamic(this, &USlottableComponent::HandleControlledComponentEndOverlap);
	Delegates.OnControlledDrop->RemoveDynamic(this, &USlottableComponent::HandleControlledComponentDrop);
	Delegates.OnControlledGrip->AddDynamic(this, &USlottableComponent::HandleControlledComponentGrip);

	USceneComponent* AttachParent = SlotComponent->GetAttachParent();

	ControlledComponent->SetSimulatePhysics(false);
	ControlledComponent->SetWorldTransform(SlotComponent->GetComponentTransform(), false, nullptr,
	                                       ETeleportType::TeleportPhysics);

	if (AttachParent != nullptr)
	{
		ControlledComponent->AttachToComponent(SlotComponent->GetAttachParent(), {EAttachmentRule::KeepWorld, false},
		                                       SlotComponent->GetAttachSocketName());

		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this]()
		{
			if (!ControlledComponent.IsValid())
				return;

			this->ControlledComponent->SetRelativeTransform(SlotComponent->GetRelativeTransform(), false, nullptr,
			                                                ETeleportType::TeleportPhysics);
			UPrimitiveComponent* RootComponent = ControlledComponent.Get();
			do
			{
				RootComponent = Cast<UPrimitiveComponent>(RootComponent->GetAttachParent());
			}
			while (RootComponent && RootComponent->GetCollisionEnabled() != ECollisionEnabled::PhysicsOnly && RootComponent->
				GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics);

			if (RootComponent != nullptr)
			{
				// Force welding even if the parent was not simulating
				FBodyInstance* ComponentBI = ControlledComponent->GetBodyInstance();
				FBodyInstance* ParentBI = RootComponent->GetBodyInstance();
				if (ComponentBI && ParentBI)
				{
					ParentBI->Weld(ComponentBI, ControlledComponent->GetComponentToWorld());
					if (bUseRigidBodyDriver)
						SetupDriver();
				}
			}
		}));
	}


	bAlreadySlotted = true;

	OnSlotted.Broadcast(SlotComponent->GetComponentTransform(), SlotComponent);
	if (IsValid(SlotComponent))
		SlotComponent->DefaultOnSlot(this);
}

bool USlottableComponent::SetupDriver()
{
	FBodyInstance* WeldBodyInstance = ControlledComponent->GetBodyInstance();
	FPhysicsActorHandle ActorHandle = WeldBodyInstance->GetPhysicsActorHandle();
	if (!FPhysicsInterface::IsValid(ActorHandle))
		return false;

	ShapesMap.Empty();
	FPhysicsCommand::ExecuteRead(ActorHandle, [&](FPhysicsActorHandle Actor)
	{
		PhysicsInterfaceTypes::FInlineShapeArray Shapes;
		FPhysicsInterface::GetAllShapes_AssumedLocked(Actor, Shapes);
		FBodyInstance* OgBodyInstance = ControlledComponent->GetBodyInstance(NAME_None, false);
		for (FPhysicsShapeHandle Shape : Shapes)
		{
			const FBodyInstance* OgShapeBodyInstance = WeldBodyInstance->GetOriginalBodyInstance(Shape);
			if (OgShapeBodyInstance != OgBodyInstance)
				continue;
#if WITH_CHAOS
			FKShapeElem* ShapeElem = FChaosUserData::Get<FKShapeElem>(FPhysicsInterface::GetUserData(Shape));
#elif PHYSICS_INTERFACE_PHYSX
			FKShapeElem* ShapeElem = FPhysxUserData::Get<FKShapeElem>(FPhysicsInterface::GetUserData(Shape));
#endif

			FTransform GlobalBase = FPhysicsInterface::GetGlobalPose_AssumesLocked(Actor);
			FTransform LocalTransform = FPhysicsInterface::GetLocalTransform(Shape);
			FTransform GlobalShape = LocalTransform * GlobalBase;
			FTransform ShapeToComponent = GlobalShape.
				GetRelativeTransform(ControlledComponent->GetComponentTransform());
			if (ShapesMap.Find(ShapeElem->GetName()))
			{
				bUsingDriver = false;
				break;
			}

			ShapesMap.Add(ShapeElem->GetName(), ShapeToComponent);
		}
	});
	return true;
}

void USlottableComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUsingDriver && bAlreadySlotted)
	{
		FPhysicsActorHandle ActorHandle = ControlledComponent->GetBodyInstance()->GetPhysicsActorHandle();
		if (FPhysicsInterface::IsValid(ActorHandle))
		{
			FPhysicsCommand::ExecuteWrite(ActorHandle, [&](FPhysicsActorHandle Actor)
			{
				PhysicsInterfaceTypes::FInlineShapeArray Shapes;
				FPhysicsInterface::GetAllShapes_AssumedLocked(Actor, Shapes);
				FTransform Global = FPhysicsInterface::GetGlobalPose_AssumesLocked(ActorHandle);

				FTransform SlotTransform = ControlledComponent->GetComponentTransform();
				SlotTransform.SetToRelativeTransform(Global);

				for (FPhysicsShapeHandle Shape : Shapes)
				{
#if WITH_CHAOS
					FKShapeElem* ShapeElem = FChaosUserData::Get<FKShapeElem>(FPhysicsInterface::GetUserData(Shape));
#elif PHYSICS_INTERFACE_PHYSX
					FKShapeElem* ShapeElem = FPhysxUserData::Get<FKShapeElem>(FPhysicsInterface::GetUserData(Shape));
#endif
					if (FTransform* LocalShapeTrans = ShapesMap.Find(ShapeElem->GetName()))
					{
						FTransform Transform = (*LocalShapeTrans) * SlotTransform;
						FPhysicsInterface::SetLocalTransform(Shape, Transform);
					}
				}
			});
		}
	}
}
