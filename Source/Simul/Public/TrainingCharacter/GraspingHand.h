#pragma once

#include "CoreMinimal.h"
#include "Misc/OptionalRepSkeletalMeshActor.h"
#include "GraspingHand.generated.h"

class UCapsuleComponent;
class UTimelineComponent;
class UHandSocketComponent;
class AVRCharacter;
class UVREPhysicalAnimationComponent;
class UVREPhysicsConstraintComponent;
class UMotionControllerComponent;
class USphereComponent;
class UCurveFloat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandAttachObject, uint8, GripId);

UENUM()
enum EFingerPart
{
	Thumb_03,
	Thumb_02,
	Index_03,
	Index_02,
	Middle_03,
	Middle_02,
	Ring_03,
	Ring_02,
	Pinky_03,
	Pinky_02,
};
ENUM_RANGE_BY_FIRST_AND_LAST(EFingerPart, EFingerPart::Thumb_03, EFingerPart::Pinky_02);

UENUM(BlueprintType)
enum class EHandLerpDirection : uint8
{
	HandToObject,
	HandToController
};

UENUM(BlueprintType)
enum class EHandAnimState : uint8
{
	Idle,
	Pointing,
	Snapshot,
	Procedural	
};

USTRUCT(BlueprintType)
struct FSelectionTraceInfo
{
	GENERATED_BODY()
	FSelectionTraceInfo() : TraceDistance(10), TraceRadius(3), PreciseTraceRadius(1.5), Offset(FVector::ZeroVector) {}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Selection")
	float TraceDistance;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Selection")
	float TraceRadius;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Selection")
	float PreciseTraceRadius;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Selection")
	FVector Offset;
};

USTRUCT(BlueprintType)
struct FSelectionHitInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	UPrimitiveComponent* Component;

	UPROPERTY(BlueprintReadWrite)
	AActor* Actor;
	
	UPROPERTY(BlueprintReadWrite)
	float Distance;
	
	UPROPERTY(BlueprintReadWrite)
	FVector ImpactPoint;

	UPROPERTY(BlueprintReadWrite)
	uint8 Priority;

	FSelectionHitInfo(): Component(nullptr), Actor(nullptr), Distance(BIG_NUMBER), ImpactPoint(FVector::ZeroVector), Priority(1) {}

	void SetForActor(AActor* InActor, float InDistance, FVector InImpactPoint, uint8 InPriority = 1)
	{
		Component = Cast<UPrimitiveComponent>(InActor->GetRootComponent());
		Actor = InActor;
		Distance = InDistance;
		ImpactPoint = InImpactPoint;
		Priority = InPriority;
	}

	void SetForComponent(UPrimitiveComponent* InComponent, float InDistance, FVector InImpactPoint, uint8 InPriority = 1)
	{
		Component= InComponent;
		Actor = nullptr;
		Distance = InDistance;
		ImpactPoint = InImpactPoint;
		Priority = InPriority;
	}
	
	FORCEINLINE void Clear()
	{
		Actor = nullptr;
		Component = nullptr;
	}

	FORCEINLINE bool IsSetForActor() { return Actor != nullptr; }
	FORCEINLINE bool IsUnset() {return Actor == nullptr && Component == nullptr; }
	FORCEINLINE UObject* GetAsObject()
	{
		return IsSetForActor() ? (UObject*) Actor : (UObject*) Component;
	}

	bool operator!=(const FSelectionHitInfo& Other) const
	{
		return Other.Component != Component; 
	}

	bool operator>(const FSelectionHitInfo& Other) const
	{
		return this->Priority > Other.Priority || (this->Priority == Other.Priority && this->Distance < Other.Distance);
	}
};

USTRUCT(BlueprintType)
struct FHandLerpInfo
{
	GENERATED_BODY()

	FHandLerpInfo() :
		LerpDirection(EHandLerpDirection::HandToObject),
		bShouldCheckPenetration(false),
		bShouldFingerGrasp(false),
		BoneName(NAME_None),
		bIsLerping(false),
		LerpAlpha(0),
		LerpSpeed(0),
		bUseInternalLerp(false),
		BaseComponent(nullptr)
	{
	}

	bool bShouldCheckPenetration;
	bool bShouldFingerGrasp;
	FName BoneName;
	
	bool bIsLerping;
	float LerpAlpha;
	float LerpSpeed;
	bool bUseInternalLerp;
	EHandLerpDirection LerpDirection;
	FTransform HandRelativeTransform;
	UPROPERTY()	
	UPrimitiveComponent* BaseComponent;
};

USTRUCT(BlueprintType)
struct FFingersInfo
{
	GENERATED_BODY()
	
	FFingersInfo() : OwningHand(nullptr)
	{
		Restart();
	}

	FFingersInfo(AGraspingHand* Hand) : OwningHand(Hand)
	{
		Restart();
	}

	UPROPERTY()	
	class AGraspingHand* OwningHand;
	TMap<TEnumAsByte<EFingerPart>, float> FingersCurl;

	void Restart();
	void Evaluate(float CulValue);
};

UCLASS(BlueprintType, Blueprintable, ClassGroup=(VRSimulation))
class SIMUL_API AGraspingHand : public AOptionalRepGrippableSkeletalMeshActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UVREPhysicsConstraintComponent* HandConstraint;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UVREPhysicalAnimationComponent* BoneDriver;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<TEnumAsByte<EFingerPart>, UCapsuleComponent*> FingerCollisionCapsules;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USphereComponent* PointFingerCollider;

	UPROPERTY(BlueprintReadOnly)
	UGripMotionControllerComponent* OwningController;
	UPROPERTY(BlueprintReadOnly)
	UGripMotionControllerComponent* OtherController;
	UPROPERTY(BlueprintReadOnly)
	AGraspingHand* OtherHand;
	UPROPERTY(BlueprintReadOnly)
	AVRCharacter* OwningCharacter;
	UPROPERTY(BlueprintReadOnly)
	USphereComponent* ControllerRoot;
	FTransform BaseTransform;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Selection")
	FSelectionTraceInfo FingerTraceInfo;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Selection")
	FSelectionTraceInfo PalmTraceInfo;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Selection")
	bool bDebugSelection;
	UPROPERTY(BlueprintReadOnly)
	FSelectionHitInfo ObjectSelected;

	// Fingers Lerp Timeline
	UPROPERTY(BlueprintReadOnly)
	UTimelineComponent* FingerLerpTimeline;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Timeline")
	UCurveFloat* FingersLerpCurve;

	// Animation
	UPROPERTY(BlueprintReadWrite)
	EHandAnimState AnimState;
	FPoseSnapshot PoseSnapshot;
	FFingersInfo FingersInfo;

	UPROPERTY(BlueprintAssignable)
	FOnHandAttachObject OnHandAttachToObject;
	UPROPERTY()
	FHandLerpInfo LerpInfo;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Hand lerp")
	float RotationDistanceSpeedMultiplier;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Hand lerp")
	float EuclideanDistanceSpeedMultiplier;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Hand lerp")
	float MaxLerpSpeed;
	
private:
	FCollisionResponseContainer OriginalHandCollision;
	uint8 CurrentGripId;
	FBPActorGripInformation* CurrentGripInfo;
	bool bIsAttached;
	
	
public:
	AGraspingHand(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;
	UFUNCTION(BlueprintCallable)
	void SetEnablePointing(bool bEnablePoint);
	UFUNCTION(BlueprintCallable)
	void ResetHand(USkeletalMesh* NewSkeletalMesh = nullptr);

protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnGrip(const FBPActorGripInformation& GripInformation);
	UFUNCTION()
	void OnDrop(const FBPActorGripInformation& GripInformation, bool bWasSocketed);
	
	UFUNCTION(BlueprintCallable)
	void StartAttachment( const EHandLerpDirection LerpDirection,UPrimitiveComponent* TargetComponent, const FTransform& InLerpRelativeTransform,
		FName BoneName = NAME_None, bool bInCheckPenetration = false, bool bShouldGrasp = false);
	void FinishAttachment();
	UFUNCTION(BlueprintCallable)
	void StartDetachment();
	
private:
	
	UFUNCTION()
	void GraspFingerTick(float Output);
	UFUNCTION()
	void OnFingerGraspFinished();
	
	void TickSelection();
	FSelectionHitInfo FindBestSelection(const TArray<FHitResult>& Hits);
		
	UFUNCTION()
	void OnOwnerTeleport();

};

