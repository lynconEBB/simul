#pragma once

#include "CoreMinimal.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InstructorScreenSubsystem.generated.h"

class FInstructorHitTester;
class AInstructorPawn;

UENUM(BlueprintType)
enum EInstructorScreenMode
{
	VrView,
	InstructorView
};

UCLASS(BlueprintType, ClassGroup=(VRSimulation))
class SIMUL_API UInstructorScreenSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	bool bShouldDisplayVrView = false;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY()
	TWeakObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent;
	
	TSharedPtr<SDPIScaler> Scaler;
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void OnViewportCreated();
	virtual void Deinitialize() override;

	
	// FTickableGameObject Interface
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UInstructorScreenSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickable() const override { return !HasAnyFlags(RF_ClassDefaultObject); }
	virtual void Tick(float DeltaTime) override;
	// End FTickableGameObject

	UFUNCTION(BlueprintCallable)
	void AddToScreen(UUserWidget* Widget);
	UFUNCTION(BlueprintCallable)
	void SetViewTarget(bool bShouldDisplayVrEye, USceneCaptureComponent2D* InSceneCaptureComponent);
	UFUNCTION(BlueprintCallable)
	UUserWidget* GetWidgetOnScreen();

private:
	FWidgetRenderer* WidgetRenderer;

	TSharedPtr<SVirtualWindow> SlateWindow;

	TSharedPtr<FInstructorHitTester> HitTester;

	
	FVector2D WidgetDrawSize;

	TWeakPtr<SViewport> InstructorViewport;

	uint32 LastFrameNumberTicked = INDEX_NONE;

	UPROPERTY()
	TWeakObjectPtr<UUserWidget> WidgetOnScreen; 
private:
	
	void TryUpdateSpectatorScreenMode();
};
