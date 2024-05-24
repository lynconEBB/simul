#include "TrainingCharacter/TrainingCharacter.h"

#include "TrainingCharacter/GraspingHand.h"
#include "SimulWorldSettings.h"
#include "TrainingCharacter/TeleporterComponent.h"
#include "Kismet/GameplayStatics.h"


ATrainingCharacter::ATrainingCharacter()
	: RightHandOffset(FTransform::Identity), LeftHandOffset(FTransform::Identity)
{
	PrimaryActorTick.bCanEverTick = true;
	
	RightTeleporter = CreateDefaultSubobject<UTeleporterComponent>(TEXT("RightTeleporter"));
	RightTeleporter->MotionSource = TEXT("RightAim");
	RightTeleporter->SetupAttachment(VRProxyComponent);
	LeftTeleporter = CreateDefaultSubobject<UTeleporterComponent>(TEXT("LeftTeleporter"));
	LeftTeleporter->MotionSource = TEXT("LeftAim");
	LeftTeleporter->SetupAttachment(VRProxyComponent);
	
	LeftControllerRoot = CreateDefaultSubobject<USphereComponent>(TEXT("LeftControllerRoot"));
	LeftControllerRoot->SetSphereRadius(4);
	RightControllerRoot = CreateDefaultSubobject<USphereComponent>(TEXT("RightControllerRoot"));
	RightControllerRoot->SetSphereRadius(4);

	LeftControllerRoot->SetupAttachment(LeftMotionController);
	RightControllerRoot->SetupAttachment(RightMotionController);
	
	HandClass = AGraspingHand::StaticClass();
	bIsTeleporting = false;
	SnapTurnMultiplier = 60;
}

void ATrainingCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WorldSettings && WorldSettings->bForcePlayerStartTransformVR)
	{
		TWeakObjectPtr<AActor> Actor = GetController<APlayerController>()->StartSpot;
		
		SetActorLocationAndRotationVR(
			Actor->GetActorLocation(),
			Actor->GetActorRotation(),
			true,true,true);
		
		FVector PlayerStartLocation = Actor->GetActorLocation();
		FVector VRLocation = GetVRLocation();
		VRLocation.Z = 140;
		PlayerStartLocation.Z = 140;
		
		
		if (VRLocation.Equals(PlayerStartLocation,1) && GetVRRotation().Equals(Actor->GetActorRotation(), 10))
		{
			WorldSettings->bForcePlayerStartTransformVR = false;
		}
	}
}

void ATrainingCharacter::BeginPlay()
{
	Super::BeginPlay();
	WorldSettings = Cast<ASimulWorldSettings>(GetWorld()->GetWorldSettings());

	checkf(HandClass, TEXT("Default Hand class cant be null"));
	AGraspingHand* DefaultObject = HandClass->GetDefaultObject<AGraspingHand>();
	checkf(DefaultObject->GetSkeletalMeshComponent()->SkeletalMesh, TEXT("Default hand class dont have a skeletal mesh asset!"));
	
	RightHand = GetWorld()->SpawnActorDeferred<AGraspingHand>(
		HandClass, RightHandOffset * RightControllerRoot->GetComponentTransform(), this,
		this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	RightHand->OwningController = RightMotionController;
	RightHand->OtherController = LeftMotionController;
	RightHand->ControllerRoot = RightControllerRoot;
	RightHand->BaseTransform = RightHandOffset;
	RightHand->FinishSpawning(FTransform::Identity, true);
	
	LeftHand = GetWorld()->SpawnActorDeferred<AGraspingHand>(
		HandClass, LeftHandOffset * LeftControllerRoot->GetComponentTransform(), this,
		this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	LeftHand->OwningController = LeftMotionController;
	LeftHand->OtherController = RightMotionController;
	LeftHand->ControllerRoot = LeftControllerRoot;
	LeftHand->BaseTransform = LeftHandOffset;
	LeftHand->FinishSpawning(FTransform::Identity, true);

	LeftHand->OtherHand = RightHand;
	RightHand->OtherHand = LeftHand;
	
	LeftTeleporter->AddActorToIgnoreTrace(LeftHand);
	LeftTeleporter->AddActorToIgnoreTrace(RightHand);
	RightTeleporter->AddActorToIgnoreTrace(RightHand);
	RightTeleporter->AddActorToIgnoreTrace(LeftHand);
	
}

void ATrainingCharacter::TryGrab(const EControllerHand Hand)
{
	AGraspingHand* GraspingHand = Hand == EControllerHand::Left ? LeftHand : RightHand;
	if (GraspingHand->ObjectSelected.IsUnset())
		return;
	UGripMotionControllerComponent* GripController = Hand == EControllerHand::Left ? LeftMotionController : RightMotionController;
	if (GripController->HasGrippedObjects())
		return;

	GraspingHand->GetSkeletalMeshComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	UObject* ToBeGriped = GraspingHand->ObjectSelected.GetAsObject();
	
	bool bIsHeld;
	TArray<FBPGripPair> HoldingControllers;
	IVRGripInterface::Execute_IsHeld(ToBeGriped, HoldingControllers, bIsHeld);
	for( FBPGripPair Pair : HoldingControllers)
	{
		FBPActorGripInformation GripInfo;
		EBPVRResultSwitch Result;
		Pair.HoldingController->GetGripByID(GripInfo, Pair.GripID, Result);
		if (!IVRGripInterface::Execute_AllowsMultipleGrips(GripInfo.GrippedObject))
		{
			Pair.HoldingController->DropObjectByInterface(nullptr, Pair.GripID);
		}
	}
	
	bool bHadSlot;
	FTransform SlotTransform;
	FName SlotName = NAME_None;

	IVRGripInterface::Execute_ClosestGripSlotInRange(ToBeGriped, GraspingHand->ObjectSelected.ImpactPoint, false, bHadSlot, SlotTransform, SlotName, GripController, NAME_None);
	EGripCollisionType GripType = IVRGripInterface::Execute_GetPrimaryGripType(GraspingHand->ObjectSelected.GetAsObject(), bHadSlot);
	FTransform GripTransform;
	if (GripType == EGripCollisionType::CustomGrip || GripType == EGripCollisionType::ManipulationGrip || !bHadSlot)
	{
		GripTransform = GraspingHand->ObjectSelected.Component->GetComponentTransform().GetRelativeTransform(GripController->GetComponentTransform());
	} else
	{
		GripTransform = GraspingHand->ObjectSelected.Component->GetComponentTransform().GetRelativeTransform(SlotTransform);
	}
	
	GripController->GripObjectByInterface(GraspingHand->ObjectSelected.GetAsObject(), GripTransform, true, NAME_None, SlotName, bHadSlot);
}

void ATrainingCharacter::TryDrop(EControllerHand Hand)
{
	UGripMotionControllerComponent* GripController = Hand == EControllerHand::Left ? LeftMotionController : RightMotionController;

	TArray<FBPActorGripInformation> Grips;
	GripController->GetAllGrips(Grips);
	
	for (const FBPActorGripInformation& Grip : Grips)
	{
		GripController->DropObjectByInterface(Grip.GrippedObject,Grip.GripID);
	}
}

void ATrainingCharacter::SetPointing(EControllerHand ControllerHand, bool bShouldPoint)
{
	AGraspingHand* GraspingHand = ControllerHand == EControllerHand::Left ? LeftHand : RightHand;
	GraspingHand->SetEnablePointing(bShouldPoint);
}

void ATrainingCharacter::HandleTrackPadClick(EControllerHand Hand)
{
	FName XAxisName = Hand == EControllerHand::Left ? "TrackpadXLeft" : "TrackpadXRight";
	FName YAxisName = Hand == EControllerHand::Left ? "TrackpadYLeft" : "TrackpadYRight";

	float XValue = InputComponent->GetAxisValue(XAxisName);
	float YValue = InputComponent->GetAxisValue(YAxisName);
	
	// ###XXX###
	// #########
	// #########
	if (YValue > 0.5 && XValue > -0.7 && XValue < 0.7)
	{
		UTeleporterComponent* Teleporter = Hand == EControllerHand::Left ? LeftTeleporter : RightTeleporter;
		Teleporter->SetTraceActive(true);
		return;
	}
	// #########
	// XXX###XXX
	// #########
	if (YValue < 0.5 && YValue > -0.5 && (XValue > 0.5 || XValue < -0.5))
		SnapTurn(XValue);
}

void ATrainingCharacter::HandleTrackpadRelease(EControllerHand Hand)
{
	UTeleporterComponent* Teleporter = Hand == EControllerHand::Left ? LeftTeleporter : RightTeleporter;
	if (Teleporter->IsTraceActive())
	{
		Teleporter->SetTraceActive(false);
		FVector Destination;
		bool bValidLocation = Teleporter->GetTeleporterTargetPosition(Destination);
		if (bValidLocation)
			TeleportTo(Destination, GetActorRotation());
	}
		
}


void ATrainingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
		
	PlayerInputComponent->BindAction<FHandActionDelegate>("GrabLeft", IE_Pressed, this, &ATrainingCharacter::TryGrab, EControllerHand::Left);
	PlayerInputComponent->BindAction<FHandActionDelegate>("GrabLeft", IE_Released, this, &ATrainingCharacter::TryDrop, EControllerHand::Left);
	PlayerInputComponent->BindAction<FHandActionDelegate>("GrabRight", IE_Pressed, this, &ATrainingCharacter::TryGrab, EControllerHand::Right);
	PlayerInputComponent->BindAction<FHandActionDelegate>("GrabRight", IE_Released, this, &ATrainingCharacter::TryDrop, EControllerHand::Right);
	
	PlayerInputComponent->BindAction<FHandActionDelegate>("TrackpadClickRight", IE_Pressed, this, &ATrainingCharacter::HandleTrackPadClick, EControllerHand::Right);
	PlayerInputComponent->BindAction<FHandActionDelegate>("TrackpadClickLeft", IE_Pressed, this, &ATrainingCharacter::HandleTrackPadClick, EControllerHand::Left);
	PlayerInputComponent->BindAction<FHandActionDelegate>("TrackpadClickLeft", IE_Released, this, &ATrainingCharacter::HandleTrackpadRelease, EControllerHand::Left);
	PlayerInputComponent->BindAction<FHandActionDelegate>("TrackpadClickRight", IE_Released, this, &ATrainingCharacter::HandleTrackpadRelease, EControllerHand::Right);
	
	PlayerInputComponent->BindAxis("TrackpadXLeft");
	PlayerInputComponent->BindAxis("TrackpadYLeft");
	PlayerInputComponent->BindAxis("TrackpadYRight");
	PlayerInputComponent->BindAxis("TrackpadXRight");

	PlayerInputComponent->BindAction<FHandEnablingDelegate>("GripLeft", IE_Pressed, this, &ATrainingCharacter::SetPointing, EControllerHand::Left, true);
	PlayerInputComponent->BindAction<FHandEnablingDelegate>("GripLeft", IE_Released, this, &ATrainingCharacter::SetPointing, EControllerHand::Left, false);
	PlayerInputComponent->BindAction<FHandEnablingDelegate>("GripRight", IE_Pressed, this, &ATrainingCharacter::SetPointing, EControllerHand::Right, true);
	PlayerInputComponent->BindAction<FHandEnablingDelegate>("GripRight", IE_Released, this, &ATrainingCharacter::SetPointing, EControllerHand::Right, false);
}

void ATrainingCharacter::SnapTurn(float SnapDirection)
{
	APlayerCameraManager* CameraManager = GetController<APlayerController>()->PlayerCameraManager;
	checkf(CameraManager, TEXT("Could not get PlayerCameraManager to fade during Teleport"));
	
	CameraManager->StartCameraFade(0,1,0.25,FLinearColor::Black, false, true);

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([CameraManager,SnapDirection, this]()
	{
		VRMovementReference->StopMovementImmediately();
		VRMovementReference->PerformMoveAction_SnapTurn(SnapDirection * SnapTurnMultiplier, EVRMoveActionVelocityRetention::VRMOVEACTION_Velocity_None, true, true);
		CameraManager->StartCameraFade(1,0,0.5,FLinearColor::Black);
	}),0.25,false);	
}

void ATrainingCharacter::TeleportTo(FVector TargetLocation, FRotator TargetRotation)
{
	if (bIsTeleporting)
		return;
	
	APlayerCameraManager* CameraManager = GetController<APlayerController>()->PlayerCameraManager;
	if (!IsValid(CameraManager))
	{
		UE_LOG(LogBlueprintUserMessages, Error, TEXT("Could not get PlayerCameraManager to fade during Teleport"));
		return;		
	}
	bIsTeleporting = true;
	CameraManager->StartCameraFade(0,1,0.25,FLinearColor::Black, false, true);
	
	FTimerHandle Handle;	
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([CameraManager,this, TargetLocation, TargetRotation]()
	{
		UVRBaseCharacterMovementComponent* MovementComponent = Cast<UVRBaseCharacterMovementComponent>(GetMovementComponent());
		MovementComponent->StopMovementImmediately();
		MovementComponent->PerformMoveAction_Teleport(GetTeleportLocation(TargetLocation), TargetRotation);
		CameraManager->StartCameraFade(1,0,0.5,FLinearColor::Black);
		this->bIsTeleporting = false;
	}),0.25,false);	
}


void ATrainingCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

}

AGraspingHand* ATrainingCharacter::GetHandByLaterality(const EControllerHand& Laterality) const
{
	return Laterality == EControllerHand::Left ? LeftHand : RightHand;
}

UGripMotionControllerComponent* ATrainingCharacter::GetOppositeController(UGripMotionControllerComponent* InController) const
{
	return InController == RightMotionController ? LeftMotionController : RightMotionController;
}