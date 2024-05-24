#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "CustomSceneCaptureComponent.generated.h"


UCLASS(ClassGroup=(VRSimulation), meta=(BlueprintSpawnableComponent))
class SIMUL_API UCustomSceneCaptureComponent : public USceneCaptureComponent2D
{
	GENERATED_BODY()

	virtual const AActor* GetViewOwner() const override;
};
