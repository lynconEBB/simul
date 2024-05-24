#include "SlotSystem/SlotComponent.h"

#include "SimulGameInstance.h"
#include "SimulGameMode.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

USlotComponent::USlotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CustomRangeTransform = FTransform::Identity;
	bCanEverAffectNavigation = false;
	UStaticMeshComponent::SetCollisionProfileName(TEXT("Slot"));
	
	SlotSourceMaterial = ConstructorHelpers::FObjectFinder<UMaterialInterface>(TEXT("/Game/Simul/Slots/M_Slot.M_Slot")).Object;
	bUsingCustomRange = false;
	CapsuleHalfHeight = 44;
	CapsuleRadius = 22;
	bIsSlotted = false;
	bIsEnable = true;
	bIsRendered = false;
	IdleColor = FLinearColor(0,0.821,1,0);
	OverlappingColor = FLinearColor(0.101,1,0,0);
}

void USlotComponent::OnRegister()
{
	Super::OnRegister();
	
	if (SlotSourceMaterial && !SlotMaterialInstance)
	{
		SlotMaterialInstance = UMaterialInstanceDynamic::Create(SlotSourceMaterial, this);
		for(int MaterialIndex = 0; MaterialIndex < GetNumMaterials(); MaterialIndex++)
		{
			SetMaterial(MaterialIndex, SlotMaterialInstance);
		}
	}
	
	SlotMaterialInstance->SetVectorParameterValue(TEXT("Color"), IdleColor);
		
	if (CustomRangeComponent == nullptr)
	{
		CustomRangeComponent = NewObject<UCapsuleComponent>(GetOwner(), *("CustomRange" + GetName()) );
		CustomRangeComponent->SetCanEverAffectNavigation(false);
		CustomRangeComponent->RegisterComponent();
		CustomRangeComponent->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
	}
	
	UpdateCustomRangeSettings();
}

#if WITH_EDITOR
void USlotComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetName() == TEXT("IdleColor") && SlotMaterialInstance)
	{
		SlotMaterialInstance->SetVectorParameterValue(TEXT("Color"), IdleColor);
	}
	
	if (CustomRangeComponent != nullptr)
		UpdateCustomRangeSettings();
}
#endif


void USlotComponent::UpdateCustomRangeSettings()
{
	CustomRangeComponent->SetCapsuleRadius(CapsuleRadius);
	CustomRangeComponent->SetCapsuleHalfHeight(CapsuleHalfHeight);
	CustomRangeComponent->SetRelativeTransform(CustomRangeTransform);
	CustomRangeComponent->SetVisibility(bUsingCustomRange);
	
	if (!bUsingCustomRange)
	{
		CustomRangeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	} else
	{
		CustomRangeComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	
	CustomRangeComponent->SetCollisionProfileName(TEXT("Slot"));
	CustomRangeComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel8, ECR_Ignore);
}

void USlotComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OnSlotBeginOverlap.AddDynamic(this, &USlotComponent::DefaultOnSlotBeginOverlap);
	OnSlotEndOverlap.AddDynamic(this, &USlotComponent::DefaultOnSlotEndOverlap);
	SlotMaterialInstance->SetVectorParameterValue(TEXT("Color"), IdleColor);

	SetSlotEnable(bIsEnable);

	ASimulGameMode* QuestGameMode = GetWorld()->GetAuthGameMode<ASimulGameMode>();
	QuestGameMode->OnShowHelperUpdate.AddDynamic(this, &USlotComponent::SetSlotVisible);
}

void USlotComponent::SetSlotEnable(bool bEnabled)
{
	bIsEnable = bEnabled;
	
	if (bIsEnable)
	{
		SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CustomRangeComponent->SetCollisionEnabled(bUsingCustomRange ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		SetVisibility(bIsRendered);
		CustomRangeComponent->SetVisibility(bIsRendered);
	} else
	{
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CustomRangeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetVisibility(false);
		CustomRangeComponent->SetVisibility(false);
	}
}

void USlotComponent::SetSlotVisible(bool bNewVisibility)
{
	bIsRendered = bNewVisibility;
	if (bIsEnable)
	{
		SetVisibility(bNewVisibility);
		CustomRangeComponent->SetVisibility(bNewVisibility);
	}
}

void USlotComponent::DestroyComponent(bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);
	CustomRangeComponent->DestroyComponent();
}

void USlotComponent::DefaultOnSlotBeginOverlap(USlottableComponent* SlottableComponent)
{
	SlotMaterialInstance->SetVectorParameterValue(TEXT("Color"), OverlappingColor);
}

void USlotComponent::DefaultOnSlotEndOverlap()
{
	SlotMaterialInstance->SetVectorParameterValue(TEXT("Color"), IdleColor);
}

void USlotComponent::DefaultOnSlot(USlottableComponent* SlottableComponent)
{
	SetSlotEnable(false);
	bIsSlotted = true;
	OnSlot.Broadcast(SlottableComponent);
}

void USlotComponent::DefaultOnUnslot(USlottableComponent* SlottableComponent)
{
	SetSlotEnable(true);
	bIsSlotted = false;
	OnUnslot.Broadcast(SlottableComponent);
}