#include "Instructor/InstructorPawn.h"

#include "Components/SceneCaptureComponent2D.h"
#include "CustomComponents/CustomSceneCaptureComponent.h"


AInstructorPawn::AInstructorPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	,SceneCaptureComponent(CreateDefaultSubobject<UCustomSceneCaptureComponent>("CaptureComponent"))
{
	PrimaryActorTick.bCanEverTick = true;
	
	SceneCaptureComponent->SetupAttachment(GetMeshComponent());
	SceneCaptureComponent->SetRelativeRotation(FRotator::ZeroRotator);
	SceneCaptureComponent->bCaptureOnMovement = false;
	SceneCaptureComponent->bCaptureEveryFrame = false;
	SceneCaptureComponent->bAlwaysPersistRenderingState = true;
	SceneCaptureComponent->CaptureSource = SCS_FinalColorLDR;
}

void AInstructorPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	GetMeshComponent()->SetWorldRotation(GetViewRotation());
}