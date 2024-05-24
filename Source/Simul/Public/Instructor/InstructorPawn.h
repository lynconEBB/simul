#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/DefaultPawn.h"
#include "InstructorPawn.generated.h"

UCLASS( ClassGroup=(VRSimulation))
class SIMUL_API AInstructorPawn : public ADefaultPawn
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UCustomSceneCaptureComponent* SceneCaptureComponent;
	
public:
	AInstructorPawn(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaTime) override;
};
