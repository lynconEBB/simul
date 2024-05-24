#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "EditorWidgetComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInitWidgetSignature);

UCLASS(ClassGroup=(VRSimulation), meta=(BlueprintSpawnableComponent))
class SIMUL_API UEditorWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
public:
	virtual void InitWidget() override;
	
	UPROPERTY(BlueprintAssignable, BlueprintReadWrite, BlueprintCallable)
	FInitWidgetSignature OnInitWidget;
};
