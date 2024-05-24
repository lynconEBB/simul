#include "WorldWidget/EditorWidgetComponent.h"

void UEditorWidgetComponent::InitWidget()
{
	Super::InitWidget();

#if WITH_EDITOR
	if (GetWidget())
	{
		GetWidget()->SetDesignerFlags(EWidgetDesignFlags::ExecutePreConstruct);
	}
#endif

	UBlueprintGeneratedClass::BindDynamicDelegates(GetOwner()->GetClass(), GetOwner());
	
	FEditorScriptExecutionGuard ScriptGuard;
	OnInitWidget.Broadcast();
}