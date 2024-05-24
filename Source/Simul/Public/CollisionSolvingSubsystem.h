#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CollisionSolvingSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSolvingInfo
{
	GENERATED_BODY()
	
	FSolvingInfo() : Component(nullptr), bUseBounds(false), bLimitToSimulated(true), IgnoreChannels({ECC_Pawn}) {}

	FSolvingInfo(
		UPrimitiveComponent* InComponent,
		const FCollisionResponseContainer& InTargetCollision,
		bool bInUseBounds = false,
		bool bInLimitToSimulated = true,
		TArray<ECollisionChannel> InIgnoreChannels = {ECC_Pawn}
	) :
		Component(InComponent),
		bUseBounds(bInUseBounds),
		bLimitToSimulated(bInLimitToSimulated),
		TargetCollision(InTargetCollision),
		IgnoreChannels(InIgnoreChannels) {}

	UPROPERTY()
	UPrimitiveComponent* Component;
	bool bUseBounds;
	bool bLimitToSimulated;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	FCollisionResponseContainer TargetCollision;
	TArray<ECollisionChannel> IgnoreChannels;

	bool operator==(const FSolvingInfo& Other) const
	{
		return Other.Component == this->Component;
	}
};

UCLASS(ClassGroup=(VRSimulation))
class SIMUL_API UCollisionSolvingSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	void UpdateCollision(FSolvingInfo Info);
	void UpdateCollision(UPrimitiveComponent* Comp1, UPrimitiveComponent* Comp2);
	void UpdateCollision(AActor* Actor1, AActor* Actor2);

private:
	virtual void Tick(float DeltaTime) override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;
	
	bool IsObjectPenetrating(const FSolvingInfo& Info);
	bool AreBoundsPenetrating(const FSolvingInfo& Info);
	
	UPROPERTY()
	TArray<FSolvingInfo> SolvingInfos;
	TArray<TPair<AActor*, AActor*>> TrackedActors;
	TArray<TPair<UPrimitiveComponent*, UPrimitiveComponent*>> TrackedComponents;
};
