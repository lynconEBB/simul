#include "TrainingCharacter/GraspingHand.h"

#include "CollisionSolvingSubsystem.h"
#include "TrainingCharacter/HandAnimInstance.h"
#include "VRCharacter.h"
#include "Components/TimelineComponent.h"
#include "Grippables/HandSocketComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/VREPhysicalAnimationComponent.h"
#include "Misc/VREPhysicsConstraintComponent.h"

#define FINGER_TIP_CHANNEL UCollisionProfile::Get()->ConvertToTraceType(ECC_GameTraceChannel11)
#define PALM_CHANNEL UCollisionProfile::Get()->ConvertToTraceType(ECC_GameTraceChannel12)

//////////////////////// Fingers /////////////////////////////////////
void FFingersInfo::Restart()
{
	for (EFingerPart Part : TEnumRange<EFingerPart>())
	{
		FingersCurl.Add(Part, 0.0);	
	}
}

void FFingersInfo::Evaluate(float CurlValue)
{
	for (EFingerPart Part : TEnumRange<EFingerPart>())
	{
		UCapsuleComponent* FingerCapsule = *OwningHand->FingerCollisionCapsules.Find(Part);
		
		if (!FingerCapsule->IsOverlappingComponent(OwningHand->LerpInfo.BaseComponent))
		{
			FingersCurl.Add(Part, CurlValue);
		}
	}
}

void AGraspingHand::GraspFingerTick(float Output)
{
	FingersInfo.Evaluate(Output);
}

void AGraspingHand::OnFingerGraspFinished()
{
	GetSkeletalMeshComponent()->GetAnimInstance()->SnapshotPose(PoseSnapshot);
	AnimState = EHandAnimState::Snapshot;
	FingersInfo.Restart();
}

///////////////////// Actor Initialization ///////////////////////////////////
AGraspingHand::AGraspingHand(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) 
{
	PrimaryActorTick.bCanEverTick = true;
	BaseTransform = FTransform::Identity;
	bDebugSelection = false;
	bIsAttached = false;
	FingersInfo = FFingersInfo(this);
	CurrentGripInfo = nullptr;
	EuclideanDistanceSpeedMultiplier = 0.4f / 40.f;
	RotationDistanceSpeedMultiplier = 0.3f / (PI * 2);
	MaxLerpSpeed = 0.5f;
	
	GetSkeletalMeshComponent()->SetGenerateOverlapEvents(true);
	GetSkeletalMeshComponent()->SetSimulatePhysics(true);
	GetSkeletalMeshComponent()->SetUseCCD(true);
	GetSkeletalMeshComponent()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetSkeletalMeshComponent()->SetAnimInstanceClass(UHandAnimInstance::StaticClass());

	BoneDriver = CreateDefaultSubobject<UVREPhysicalAnimationComponent>("BoneDriver");
	
	HandConstraint = CreateDefaultSubobject<UVREPhysicsConstraintComponent>("HandConstraint");
	HandConstraint->SetupAttachment(GetRootComponent());
	HandConstraint->SetLinearXLimit(LCM_Free,0);
	HandConstraint->SetLinearYLimit(LCM_Free,0);
	HandConstraint->SetLinearZLimit(LCM_Free,0);
	HandConstraint->SetLinearDriveParams(10000,3000,10000);
	HandConstraint->SetLinearPositionDrive(true, true, true);
	HandConstraint->SetLinearVelocityDrive(true, true, true);
	HandConstraint->SetAngularDriveMode(EAngularDriveMode::SLERP);
	HandConstraint->SetAngularDriveParams(1200000.0,90000,1200000.0);
	HandConstraint->SetAngularVelocityDriveSLERP(true);
	HandConstraint->SetOrientationDriveSLERP(true);

	FingerLerpTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("FingerLerpTimeline"));
	FingerLerpTimeline->SetLooping(false);
	FingerLerpTimeline->SetTimelineLength(0.5f);
	FingerLerpTimeline->SetPlaybackPosition(0.0f, false);
	FingerLerpTimeline->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	for (EFingerPart FingerPart : TEnumRange<EFingerPart>())
	{
		FString EnumString = UEnum::GetValueAsString(FingerPart);
		UCapsuleComponent* FingerCapsule = CreateDefaultSubobject<UCapsuleComponent>(*EnumString);
		FingerCapsule->SetCollisionProfileName("Finger");

		FingerCapsule->SetupAttachment(GetRootComponent(), *EnumString.ToLower().Append("_r"));
		FingerCapsule->ShapeColor = FColor::Emerald;
		FingerCapsule->SetCapsuleHalfHeight(1.5);
		FingerCapsule->SetCapsuleRadius(0.5);
		FingerCollisionCapsules.Add(FingerPart, FingerCapsule);
	}

	PointFingerCollider = CreateDefaultSubobject<USphereComponent>(TEXT("PointFingerCollider"));
	PointFingerCollider->SetupAttachment(GetRootComponent());
	PointFingerCollider->SetSphereRadius(0.9);
	PointFingerCollider->ComponentTags.Add(TEXT("PointCollider"));
	PointFingerCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PointFingerCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	PointFingerCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
}

void AGraspingHand::BeginPlay()
{
	Super::BeginPlay();
	OriginalHandCollision = GetSkeletalMeshComponent()->GetCollisionResponseToChannels();
	OwningCharacter = Cast<AVRCharacter>(GetOwner());
	checkf(OwningCharacter, TEXT("Grasping hand owner needs to be a VRCharacter!"));

	this->AddTickPrerequisiteComponent(OwningController);
	GetSkeletalMeshComponent()->AddTickPrerequisiteComponent(OwningCharacter->VRMovementReference);
	BoneDriver->AddTickPrerequisiteComponent(OwningCharacter->VRMovementReference);
	BoneDriver->AddTickPrerequisiteComponent(GetSkeletalMeshComponent());

	HandConstraint->SetWorldLocation(GetSkeletalMeshComponent()->GetCenterOfMass());
	HandConstraint->SetConstrainedComponents(ControllerRoot, NAME_None, GetSkeletalMeshComponent(), TEXT("hand_r"));
	HandConstraint->SetConstraintToForceBased(true);
	
	BoneDriver->SetSkeletalMeshComponent(GetSkeletalMeshComponent());
	BoneDriver->SetupWeldedBoneDriver({TEXT("hand_r")});
	
	FOnTimelineFloat onFingersTimelineCallback;
	onFingersTimelineCallback.BindDynamic(this, &AGraspingHand::GraspFingerTick);
	FingerLerpTimeline->AddInterpFloat(FingersLerpCurve, onFingersTimelineCallback);
	FOnTimelineEvent onFingersTimelineFinishedCallback;
	onFingersTimelineFinishedCallback.BindDynamic(this, &AGraspingHand::OnFingerGraspFinished);
	FingerLerpTimeline->SetTimelineFinishedFunc(onFingersTimelineFinishedCallback);
	
	OwningCharacter->OnCharacterTeleported_Bind.AddDynamic(this, &AGraspingHand::OnOwnerTeleport);
	OwningCharacter->OnCharacterNetworkCorrected_Bind.AddDynamic(this, &AGraspingHand::OnOwnerTeleport);
	
	PointFingerCollider->AttachToComponent(GetSkeletalMeshComponent(), FAttachmentTransformRules(EAttachmentRule::KeepWorld,true), TEXT("index_03_r"));

	OwningController->OnGrippedObject.AddDynamic(this, &AGraspingHand::OnGrip);
	OwningController->OnDroppedObject.AddDynamic(this, &AGraspingHand::OnDrop);
}

/////////////////// Grip / Lerp Hand /////////////////////////////////////////
void AGraspingHand::OnGrip(const FBPActorGripInformation& GripInformation)
{
	if (GripInformation.GripCollisionType == EGripCollisionType::EventsOnly ||
		(LerpInfo.bIsLerping && LerpInfo.LerpDirection == EHandLerpDirection::HandToObject) || bIsAttached)
		return;
	UHandSocketComponent* HandSocket = UHandSocketComponent::GetHandSocketComponentFromObject(GripInformation.GrippedObject, GripInformation.SlotName);
	if (!IsValid(HandSocket)) 
		return;

	CurrentGripInfo = OwningController->GetGripPtrByID(GripInformation.GripID);
	CurrentGripId = GripInformation.GripID;
	
	UPrimitiveComponent* GrippedComponent = CurrentGripInfo->GripTargetType == EGripTargetType::ActorGrip
		? Cast<UPrimitiveComponent>(CurrentGripInfo->GetGrippedActor()->GetRootComponent())
		: CurrentGripInfo->GetGrippedComponent();
	
	bool bHasCustomAnim = HandSocket->GetBlendedPoseSnapShot(PoseSnapshot, GetSkeletalMeshComponent(), false, false);
	EControllerHand HandType;
	OwningController->GetHandType(HandType);
	const FTransform HandRelativeTransform = HandSocket->GetMeshRelativeTransform(HandType == EControllerHand::Right);

	StartAttachment(EHandLerpDirection::HandToObject, GrippedComponent, HandRelativeTransform, HandSocket->GetAttachSocketName(), false, !bHasCustomAnim);
}

bool ObjectLerpDuringGrip(const FBPActorGripInformation* GripInfo)
{
	return GripInfo->GripCollisionType != EGripCollisionType::CustomGrip && GripInfo->GripCollisionType != EGripCollisionType::ManipulationGrip;
}

void AGraspingHand::StartAttachment(const EHandLerpDirection LerpDirection, UPrimitiveComponent* TargetComponent, const FTransform& InLerpRelativeTransform, FName BoneName, bool bInCheckPenetration, bool bShouldGrasp)
{
	DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
	GetSkeletalMeshComponent()->SetSimulatePhysics(false);
	HandConstraint->BreakConstraint();
	GetSkeletalMeshComponent()->SetCollisionResponseToAllChannels(ECR_Overlap);
	
	if (LerpDirection == EHandLerpDirection::HandToObject) {
		UVRExpansionFunctionLibrary::SetObjectsIgnoreCollision(this, GetSkeletalMeshComponent(),
		NAME_None, false, TargetComponent, NAME_None, false, true);	
		AnimState = bShouldGrasp ? EHandAnimState::Procedural : EHandAnimState::Snapshot;
				
		LerpInfo.bUseInternalLerp = CurrentGripInfo == nullptr || !ObjectLerpDuringGrip(CurrentGripInfo);
	} else
	{
		LerpInfo.bUseInternalLerp = true;
	}

	LerpInfo.BaseComponent = TargetComponent;
	LerpInfo.BoneName = BoneName;
	LerpInfo.HandRelativeTransform = InLerpRelativeTransform;
	LerpInfo.bShouldCheckPenetration = bInCheckPenetration;
	LerpInfo.LerpDirection = LerpDirection;
	LerpInfo.bShouldFingerGrasp = bShouldGrasp;
	
	const UVRGlobalSettings* VrSettings = GetDefault<UVRGlobalSettings>();
	LerpInfo.LerpSpeed = 1.f / VrSettings->LerpDuration;
	LerpInfo.LerpAlpha = 0.f;
	LerpInfo.bIsLerping = true;	
}

void AGraspingHand::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	TickSelection();
	
	if (LerpInfo.bIsLerping)
	{
		if (LerpInfo.bUseInternalLerp)
		{
			LerpInfo.LerpAlpha += DeltaTime * LerpInfo.LerpSpeed;	
		} else {
			LerpInfo.LerpAlpha = CurrentGripInfo->bIsLerping ? CurrentGripInfo->CurrentLerpTime : 1.f;		
		}
		float Alpha = LerpInfo.LerpAlpha;

		FTransform Start = GetSkeletalMeshComponent()->GetComponentTransform();
		FTransform AttachBaseTransform;
		
		if (!LerpInfo.bUseInternalLerp && CurrentGripInfo)
		{
			AttachBaseTransform = CurrentGripInfo->RelativeTransform * OwningController->GetComponentTransform();
		} else
		{
			AttachBaseTransform = LerpInfo.BoneName != NAME_None
				? LerpInfo.BaseComponent->GetSocketTransform(LerpInfo.BoneName)
				: LerpInfo.BaseComponent->GetComponentTransform();
		}
		
		FTransform End = LerpInfo.HandRelativeTransform * AttachBaseTransform;
		FTransform CurrentTransform = UKismetMathLibrary::TLerp(Start,End, Alpha, ELerpInterpolationMode::QuatInterp);
		SetActorLocationAndRotation(CurrentTransform.GetLocation(), CurrentTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
		
		if (Alpha >= 1.f)
		{
			if (!LerpInfo.bUseInternalLerp)
			{
				FTransform PredictedTransform = CurrentGripInfo->RelativeTransform * OwningController->GetComponentTransform();
				FTransform RealTransform = LerpInfo.BaseComponent->GetComponentTransform();
				float AngularDistance = PredictedTransform.GetRotation().AngularDistance(RealTransform.GetRotation());
				float EuclideanDist = FVector::Dist(PredictedTransform.GetLocation(), RealTransform.GetLocation());
				
				float LerpDuration =  FMath::Max(RotationDistanceSpeedMultiplier * AngularDistance, EuclideanDistanceSpeedMultiplier * EuclideanDist);
				LerpDuration = FMath::Clamp(LerpDuration, 0.f, MaxLerpSpeed);
				
				LerpInfo.LerpSpeed = 1.f / LerpDuration;
				LerpInfo.LerpAlpha = 0.0f;
				LerpInfo.bUseInternalLerp = true;
			} else
			{
				FinishAttachment();
			}	
		}
	}
}

void AGraspingHand::FinishAttachment()
{
	LerpInfo.bIsLerping = false;
	LerpInfo.LerpAlpha = 0.f;
	
	if (LerpInfo.LerpDirection == EHandLerpDirection::HandToObject)
	{
		AttachToComponent(LerpInfo.BaseComponent, FAttachmentTransformRules(EAttachmentRule::KeepWorld, true), LerpInfo.BoneName);
		OnHandAttachToObject.Broadcast(CurrentGripId);
		bIsAttached = true;
	} else
	{
		GetSkeletalMeshComponent()->SetSimulatePhysics(true);
		HandConstraint->SetConstrainedComponents(ControllerRoot, NAME_None, GetSkeletalMeshComponent(), TEXT("hand_r"));
		HandConstraint->SetConstraintToForceBased(true);
	}
	BoneDriver->RefreshWeldedBoneDriver();
	
	if (LerpInfo.bShouldCheckPenetration)
	{
		UCollisionSolvingSubsystem* CollisionSolvingSubsystem = GetWorld()->GetSubsystem<UCollisionSolvingSubsystem>();
		FSolvingInfo SolvingInfo(GetSkeletalMeshComponent(), OriginalHandCollision, true, false, {ECC_Pawn, ECC_WorldStatic});
		CollisionSolvingSubsystem->UpdateCollision(SolvingInfo);
	} else
	{
		GetSkeletalMeshComponent()->SetCollisionResponseToChannels(OriginalHandCollision);	
	}
	
	if (LerpInfo.bShouldFingerGrasp)
		FingerLerpTimeline->PlayFromStart();		
}

void AGraspingHand::OnDrop(const FBPActorGripInformation& GripInformation, bool bWasSocketed)
{
	if (GripInformation.GripID != CurrentGripId)
		return;

	CurrentGripInfo = nullptr;
	CurrentGripId = 0;
	
	StartDetachment();
}

void AGraspingHand::StartDetachment()
{
	bIsAttached = false;
	
	GetWorld()->GetSubsystem<UCollisionSolvingSubsystem>()->UpdateCollision(GetSkeletalMeshComponent(), LerpInfo.BaseComponent);
	StartAttachment(EHandLerpDirection::HandToController, OwningController, BaseTransform, NAME_None, true);
	
	FingersInfo.Restart();
	FingerLerpTimeline->Stop();
	AnimState = EHandAnimState::Idle;
}

////////////////////////// Teleportation / Snap Turn adjustments ///////////////////////////
void AGraspingHand::OnOwnerTeleport()
{
	if (OwningController->HasGrippedObjects())
		return;
	
	GetSkeletalMeshComponent()->SetCollisionResponseToAllChannels(ECR_Overlap);
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this](){
		ResetHand();

		FSolvingInfo SolvingInfo(GetSkeletalMeshComponent(), OriginalHandCollision, true, false);
		GetWorld()->GetSubsystem<UCollisionSolvingSubsystem>()->UpdateCollision(SolvingInfo);
    }));
}

void AGraspingHand::ResetHand(USkeletalMesh* NewSkeletalMesh)
{
	GetSkeletalMeshComponent()->SetSimulatePhysics(false);
	if (NewSkeletalMesh)
		GetSkeletalMeshComponent()->SetSkeletalMesh(NewSkeletalMesh, true);
	GetSkeletalMeshComponent()->SetWorldTransform(BaseTransform * OwningController->GetComponentTransform(),false, nullptr, ETeleportType::TeleportPhysics);
	GetSkeletalMeshComponent()->AttachToComponent(OwningController, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
	GetSkeletalMeshComponent()->SetSimulatePhysics(true);
	if (NewSkeletalMesh)
		BoneDriver->RefreshWeldedBoneDriver(false);
	HandConstraint->SetConstrainedComponents(ControllerRoot, NAME_None, GetSkeletalMeshComponent(), TEXT("hand_r"));
}

void AGraspingHand::SetEnablePointing(bool bEnablePoint)
{
	if (AnimState != EHandAnimState::Idle && AnimState != EHandAnimState::Pointing)
		return;
		
	if (bEnablePoint)
	{
		AnimState = EHandAnimState::Pointing;
		PointFingerCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	} else
	{
		AnimState = EHandAnimState::Idle;
		PointFingerCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

//////////////////////////////// Object selection trace /////////////////////////////////////
void AGraspingHand::TickSelection()
{
	if (OwningController->HasGrippedObjects() || AnimState == EHandAnimState::Pointing)
	{
		if (!ObjectSelected.IsUnset() && OtherHand->ObjectSelected != ObjectSelected)
		{
			ObjectSelected.Component->SetRenderCustomDepth(false);
			ObjectSelected.Clear();
		}
		return;
	}

	FVector FingerTracePosition = GetSkeletalMeshComponent()->GetComponentTransform().TransformPosition(FingerTraceInfo.Offset);
	FVector PalmTracePosition = GetSkeletalMeshComponent()->GetComponentTransform().TransformPosition(PalmTraceInfo.Offset);
	EControllerHand ControllerHand;
	OwningController->GetHandType(ControllerHand);
	FVector UpVector = ControllerHand == EControllerHand::Left
		? GetSkeletalMeshComponent()->GetUpVector()
		: -GetSkeletalMeshComponent()->GetUpVector();
	
	TArray<FHitResult> FingerResults;
	UKismetSystemLibrary::SphereTraceMulti(this, FingerTracePosition,
		FingerTracePosition + GetSkeletalMeshComponent()->GetForwardVector() * FingerTraceInfo.TraceDistance, FingerTraceInfo.PreciseTraceRadius,
		FINGER_TIP_CHANNEL, false, {OwningCharacter},
		bDebugSelection ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, FingerResults, true);

	TArray<FHitResult> PalmResults;
	UKismetSystemLibrary::SphereTraceMulti(this, PalmTracePosition,
		PalmTracePosition + UpVector * PalmTraceInfo.TraceDistance, PalmTraceInfo.PreciseTraceRadius,
		PALM_CHANNEL, false, {OwningCharacter},
		bDebugSelection ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,PalmResults, true);
	
	// If we dont get results in the precise trace try with a bigger one
	FSelectionHitInfo FingerMatch = FindBestSelection(FingerResults);
	if (FingerMatch.IsUnset())
	{
		UKismetSystemLibrary::SphereTraceMulti(this, FingerTracePosition,
			FingerTracePosition + GetSkeletalMeshComponent()->GetForwardVector() * FingerTraceInfo.TraceDistance, FingerTraceInfo.TraceRadius,
			FINGER_TIP_CHANNEL, false, {OwningCharacter},
			bDebugSelection ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, FingerResults, true);
		FingerMatch = FindBestSelection(FingerResults);
	}

	// If we dont get results in the precise trace try with a bigger one
	FSelectionHitInfo PalmMatch = FindBestSelection(PalmResults);
	if (PalmMatch.IsUnset())
	{
		UKismetSystemLibrary::SphereTraceMulti(this, PalmTracePosition,
			PalmTracePosition + UpVector * PalmTraceInfo.TraceDistance, PalmTraceInfo.TraceRadius,
			PALM_CHANNEL, false, {OwningCharacter},
			bDebugSelection ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,PalmResults, true);
		PalmMatch = FindBestSelection(PalmResults);
	}

	FSelectionHitInfo BestMatch = FingerMatch > PalmMatch ? FingerMatch : PalmMatch;
	
	if (BestMatch.IsUnset())
	{
		if (!ObjectSelected.IsUnset() && OtherHand->ObjectSelected != ObjectSelected)
			ObjectSelected.Component->SetRenderCustomDepth(false);
		ObjectSelected.Clear();
	}

	if (BestMatch != ObjectSelected)
	{
		if (!ObjectSelected.IsUnset() && OtherHand->ObjectSelected != ObjectSelected)
			ObjectSelected.Component->SetRenderCustomDepth(false);
		if (BestMatch.Component->CustomDepthStencilValue == 0)
			BestMatch.Component->SetCustomDepthStencilValue(1);
		BestMatch.Component->SetRenderCustomDepth(true);
	}
	
	ObjectSelected = BestMatch;
}

FSelectionHitInfo AGraspingHand::FindBestSelection(const TArray<FHitResult>& Hits) 
{
	bool bFirst = true;
	FSelectionHitInfo BestMatch;
	
	for (const FHitResult& Result : Hits)
	{
		FSelectionHitInfo CurrentSelection;
		
		if (Result.Component->Implements<UVRGripInterface>())
			CurrentSelection.SetForComponent(Result.GetComponent(), Result.Distance, Result.ImpactPoint);	
		else if (Result.Actor->Implements<UVRGripInterface>())
			CurrentSelection.SetForActor(Result.GetActor(), Result.Distance, Result.ImpactPoint);

		if (CurrentSelection.IsUnset() || IVRGripInterface::Execute_DenyGripping(CurrentSelection.GetAsObject(), OwningController))
			continue;
			
		const FBPAdvGripSettings& AdvancedSettings = IVRGripInterface::Execute_AdvancedGripSettings(CurrentSelection.GetAsObject());
		CurrentSelection.Priority = AdvancedSettings.GripPriority;
		
		if (bFirst || CurrentSelection > BestMatch) 
		{
			bFirst = false;
			BestMatch = CurrentSelection;
		}
	}
	
	return BestMatch;
}