#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldWidget.generated.h"

class UEditorWidgetComponent;

UCLASS(ClassGroup=(VRSimulation))
class SIMUL_API AWorldWidget : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UEditorWidgetComponent* WidgetComponent;	

public:
	AWorldWidget();

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif
	
};
