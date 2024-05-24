#include "CustomComponents/CustomSceneCaptureComponent.h"

const AActor* UCustomSceneCaptureComponent::GetViewOwner() const
{
	return GetOwner();
}
