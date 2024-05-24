#include "Instructor/InstructorScreenSubsystem.h"

#include "IHeadMountedDisplay.h"
#include "ISpectatorScreenController.h"
#include "IXRTrackingSystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/UserInterfaceSettings.h"
#include "Input/HittestGrid.h"
#include "Widgets/SViewport.h"
#include "Widgets/Layout/SDPIScaler.h"

class FInstructorHitTester : public ICustomHitTestPath
{
public:
	FInstructorHitTester(const TWeakPtr<SVirtualWindow>& SlateWindow, FVector2D WidgetSize)
		: VirtualWindow(SlateWindow)
		  , WidgetDrawSize(WidgetSize)
		  , LastLocalHitLocation(FVector2D::ZeroVector)
	{
	}

	virtual TArray<FWidgetAndPointer> GetBubblePathAndVirtualCursors(
		const FGeometry& InGeometry,
		FVector2D DesktopSpaceCoordinate,
		bool bIgnoreEnabledStatus) const override
	{
		TArray<FWidgetAndPointer> ArrangedWidgets;

		if (TSharedPtr<SVirtualWindow> SlateWindowPin = VirtualWindow.Pin())
		{
			FVector2D LocalMouseCoordinate = InGeometry.AbsoluteToLocal(DesktopSpaceCoordinate);
			float CursorRadius = 0.f;
			ArrangedWidgets = SlateWindowPin->GetHittestGrid().GetBubblePath(
				LocalMouseCoordinate, CursorRadius, bIgnoreEnabledStatus);

			FVirtualPointerPosition VirtualMouseCoordinate(LocalMouseCoordinate, LastLocalHitLocation);
			LastLocalHitLocation = LocalMouseCoordinate;

			for (FWidgetAndPointer& ArrangedWidget : ArrangedWidgets)
			{
				if (ArrangedWidget.PointerPosition.IsValid())
					*ArrangedWidget.PointerPosition = VirtualMouseCoordinate;
			}
		}

		return ArrangedWidgets;
	}

	virtual void ArrangeCustomHitTestChildren(
		FArrangedChildren& ArrangedChildren) const override
	{
		if (TSharedPtr<SVirtualWindow> SlateWindowPin = VirtualWindow.Pin())
		{
			FGeometry WidgetGeometry;
			ArrangedChildren.AddWidget(
				FArrangedWidget(
					SlateWindowPin.ToSharedRef(),
					WidgetGeometry.MakeChild(WidgetDrawSize, FSlateLayoutTransform())
				)
			);
		}
	}

	virtual TSharedPtr<FVirtualPointerPosition> TranslateMouseCoordinateForCustomHitTestChild(
		const TSharedRef<SWidget>& ChildWidget,
		const FGeometry& ViewportGeometry,
		const FVector2D& ScreenSpaceMouseCoordinate,
		const FVector2D& LastScreenSpaceMouseCoordinate) const override
	{
		return MakeShared<FVirtualPointerPosition>();
	}

	void SetWidgetDrawSize(FVector2D NewWidgetDrawSize)
	{
		WidgetDrawSize = NewWidgetDrawSize;
	}

private:
	TWeakPtr<SVirtualWindow> VirtualWindow;
	FVector2D WidgetDrawSize;
	mutable FVector2D LastLocalHitLocation;
};


void UInstructorScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	GetGameInstance()->GetGameViewportClient()->OnViewportCreated().AddUObject(this, &UInstructorScreenSubsystem::OnViewportCreated);
}

void UInstructorScreenSubsystem::OnViewportCreated()
{
	WidgetDrawSize = GEngine->GameViewport->GetWindow()->GetViewportSize();
	WidgetRenderer = new FWidgetRenderer(true, false);
	WidgetRenderer->SetIsPrepassNeeded(true);

	RenderTarget = NewObject<UTextureRenderTarget2D>(this, NAME_None, RF_Transient);
	RenderTarget->ClearColor = FLinearColor::Transparent;
	RenderTarget->InitCustomFormat(WidgetDrawSize.X, WidgetDrawSize.Y,
		FSlateApplication::Get().GetRenderer()->GetSlateRecommendedColorFormat(), false);
	RenderTarget->UpdateResourceImmediate();
	
	InstructorViewport = GEngine->GetGameViewportWidget();
	
	SlateWindow = SNew(SVirtualWindow).Size(WidgetDrawSize);
	Scaler = SNew(SDPIScaler).DPIScale(1);
	
	SlateWindow->SetContent(Scaler.ToSharedRef());
	SlateWindow->SetIsFocusable(true);
	SlateWindow->SetVisibility(EVisibility::Visible);

	if(FSlateApplication::Get().IsInitialized())
		FSlateApplication::Get().RegisterVirtualWindow(SlateWindow.ToSharedRef());
	
	HitTester = MakeShared<FInstructorHitTester>(SlateWindow, WidgetDrawSize);
	InstructorViewport.Pin()->SetCustomHitTestPath(HitTester);
	SlateWindow->AssignParentWidget(InstructorViewport.Pin());

	TryUpdateSpectatorScreenMode();
}

void UInstructorScreenSubsystem::Tick(float DeltaTime)
{
	if (LastFrameNumberTicked == GFrameCounter)
		return;

	FVector2D CurrentSize = GetGameInstance()->GetGameViewportClient()->GetWindow()->GetSizeInScreen();
	if (CurrentSize != WidgetDrawSize)
	{
		if (Scaler.IsValid())
		{
			const UUserInterfaceSettings* UISettings = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass());
			const FRichCurve* DPICurve = UISettings->UIScaleCurve.GetRichCurveConst();
			float ShortestSide = CurrentSize.X < CurrentSize.Y ? CurrentSize.X : CurrentSize.Y;
			float NewDPI = DPICurve->Eval(ShortestSide, 1.0f);
			Scaler->SetDPIScale(NewDPI);
		}
		HitTester->SetWidgetDrawSize(CurrentSize);
		RenderTarget->ResizeTarget(CurrentSize.X, CurrentSize.Y);
		WidgetDrawSize = CurrentSize;
	}
	
	RenderTarget->UpdateResourceImmediate();

	if (!bShouldDisplayVrView && SceneCaptureComponent.IsValid())
		SceneCaptureComponent->CaptureScene();

	if (SlateWindow != nullptr)
	{
		WidgetRenderer->DrawWindow(
			RenderTarget,
			SlateWindow->GetHittestGrid(),
			SlateWindow.ToSharedRef(),
			1.f, WidgetDrawSize,
			DeltaTime);
	}

	LastFrameNumberTicked = GFrameCounter;
}

void UInstructorScreenSubsystem::AddToScreen(UUserWidget* Widget)
{
	if (!IsValid(Widget))
	{
		SlateWindow->SetContent(SNullWidget::NullWidget);
		return;
	}

	Scaler->SetContent(Widget->TakeWidget());

	WidgetOnScreen = Widget;
}

void UInstructorScreenSubsystem::SetViewTarget(bool bShouldDisplayVrEye,
                                               USceneCaptureComponent2D* InSceneCaptureComponent)
{
	bShouldDisplayVrView = bShouldDisplayVrEye;
	if (IsValid(InSceneCaptureComponent))
	{
		SceneCaptureComponent = InSceneCaptureComponent;
		SceneCaptureComponent->TextureTarget = RenderTarget;
	}
	
	RenderTarget->UpdateResourceImmediate();
	TryUpdateSpectatorScreenMode();
}

void UInstructorScreenSubsystem::TryUpdateSpectatorScreenMode()
{
	IHeadMountedDisplay* HMD = GEngine->XRSystem.IsValid() ? GEngine->XRSystem->GetHMDDevice() : nullptr;
	ISpectatorScreenController* Controller = HMD ? HMD->GetSpectatorScreenController() : nullptr;
	
	if (Controller)
	{
		FSpectatorScreenModeTexturePlusEyeLayout Layout;
		Layout.TextureRectMin = FVector2D::ZeroVector;
		Layout.TextureRectMax = FVector2D::UnitVector;
		Layout.bUseAlpha = true;
		Controller->SetSpectatorScreenModeTexturePlusEyeLayout(Layout);
		Controller->SetSpectatorScreenTexture(RenderTarget);
		
		if (bShouldDisplayVrView)		
			Controller->SetSpectatorScreenMode(ESpectatorScreenMode::TexturePlusEye);
		else
			Controller->SetSpectatorScreenMode(ESpectatorScreenMode::Texture);
	}
}

void UInstructorScreenSubsystem::Deinitialize()
{
	if (WidgetRenderer)
	{
		BeginCleanup(WidgetRenderer);
		WidgetRenderer = nullptr;
	}
	
	FSlateApplication::Get().UnregisterVirtualWindow(SlateWindow.ToSharedRef());
	if (InstructorViewport.IsValid())
		InstructorViewport.Pin()->SetCustomHitTestPath(nullptr);

	InstructorViewport.Reset();
	HitTester.Reset();
	Scaler.Reset();
	SlateWindow.Reset();
}

UUserWidget* UInstructorScreenSubsystem::GetWidgetOnScreen()
{
	return WidgetOnScreen.Get();
}
