#include "TrainingCharacter/TeleporterComponent.h"

#include "NavigationSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "VRCharacter.h"
#include "Kismet/GameplayStatics.h"

UTeleporterComponent::UTeleporterComponent() : CharacterOwner(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;

	bHasValidTeleportLocation = false;
	LaunchSpeedMultiplier = 650;
	ProjectileRadius = 3.6;
	NavZOffset = 8;
	bTraceDebug = false;
	ValidColor = FLinearColor::Green;
	InvalidColor = FLinearColor::Red;
}

void UTeleporterComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner = Cast<AVRCharacter>(GetOwner());
	checkf(CharacterOwner, TEXT("Teleporter needs to be owned by a VRCharacter!"));

	TraceEffectComponent = NewObject<UNiagaraComponent>(this, TEXT("TraceEffectComponent"));
	TraceEffectComponent->SetAsset(TraceEffectSystem);
	TraceEffectComponent->SetVisibility(false);
	TraceEffectComponent->RegisterComponent();
	TraceEffectComponent->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
	
	RingEffectComponent = NewObject<UNiagaraComponent>(this, TEXT("RingEffectComponent"));
	RingEffectComponent->SetAsset(RingEffectSystem);
	RingEffectComponent->SetVisibility(false);
	RingEffectComponent->RegisterComponent();
	TraceEffectComponent->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
}

void UTeleporterComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!bTraceActive)
		return;

	FPredictProjectilePathParams ProjectilePathParams;
	ProjectilePathParams.bTraceWithCollision = true;
	ProjectilePathParams.bTraceWithChannel = false;
	ProjectilePathParams.ProjectileRadius = ProjectileRadius;
	ProjectilePathParams.StartLocation = this->GetComponentLocation();
	ProjectilePathParams.LaunchVelocity = this->GetForwardVector() * LaunchSpeedMultiplier;
	ProjectilePathParams.ObjectTypes.Add(TEnumAsByte<EObjectTypeQuery>(ECC_WorldStatic));
	ProjectilePathParams.ActorsToIgnore = ActorsToIgnoreDuringTrace;
	ProjectilePathParams.ActorsToIgnore.Add(CharacterOwner);
	ProjectilePathParams.DrawDebugType = bTraceDebug ? EDrawDebugTrace::Type::ForDuration : EDrawDebugTrace::Type::None;
	ProjectilePathParams.DrawDebugTime = 0.05;
	ProjectilePathParams.MaxSimTime = 2;
	ProjectilePathParams.SimFrequency = 30;
	FPredictProjectilePathResult ProjectilePathResult;
	UGameplayStatics::PredictProjectilePath(this, ProjectilePathParams, ProjectilePathResult);
	
	TArray<FVector> PathLocations;
	for (FPredictProjectilePathPointData PathData : ProjectilePathResult.PathData)
	{
		PathLocations.Add(PathData.Location);
	}

	FNavLocation ProjectedLocation;
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	
	bool bProjectSuccess = NavSys->ProjectPointToNavigation(ProjectilePathResult.HitResult.Location, ProjectedLocation);
	ProjectedLocation.Location.Z -= NavZOffset;
	bool bProjectedDown = ProjectedLocation.Location.Z <= ProjectilePathResult.HitResult.Location.Z;
	
	Destination = ProjectedLocation;
	bool bCanTeleport = bProjectSuccess && bProjectedDown && ProjectilePathResult.HitResult.bBlockingHit;
	
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(TraceEffectComponent, TEXT("User.PointArray"), PathLocations);
	if (bHasValidTeleportLocation != bCanTeleport)
		RingEffectComponent->SetVisibility(bCanTeleport);
		TraceEffectComponent->SetColorParameter(TEXT("Color"), bCanTeleport ? ValidColor : InvalidColor);
	
	if (bCanTeleport)
		RingEffectComponent->SetWorldLocation(ProjectedLocation);

	bHasValidTeleportLocation = bCanTeleport;
}

void UTeleporterComponent::AddActorToIgnoreTrace(AActor* Actor)
{
	ActorsToIgnoreDuringTrace.Add(Actor);	
}

void UTeleporterComponent::SetTraceActive(bool bInActive)
{
	if (bTraceActive != bInActive)
	{
		RingEffectComponent->SetVisibility(bInActive);
		TraceEffectComponent->SetVisibility(bInActive);
		bTraceActive = bInActive;
	}
}

bool UTeleporterComponent::GetTeleporterTargetPosition(FVector& OutPosition)
{
	OutPosition = Destination;
	return bHasValidTeleportLocation;
}
