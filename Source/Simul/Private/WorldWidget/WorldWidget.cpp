
#include "WorldWidget/WorldWidget.h"
#include "WorldWidget//EditorWidgetComponent.h"


AWorldWidget::AWorldWidget()
{
	PrimaryActorTick.bCanEverTick = true;
	WidgetComponent = CreateDefaultSubobject<UEditorWidgetComponent>(TEXT("WidgetComponent"));
	SetRootComponent(WidgetComponent);
}

#if WITH_EDITOR
void AWorldWidget::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	
	ReregisterAllComponents();
}
#endif

