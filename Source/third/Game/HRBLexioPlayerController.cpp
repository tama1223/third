// Copyright HRB Lexio Project. All Rights Reserved.

#include "HRBLexioPlayerController.h"
#include "UI/HRBLexioHUD.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HRBLexioPlayerController)

AHRBLexioPlayerController::AHRBLexioPlayerController()
{
}

void AHRBLexioPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Show mouse cursor and set input mode to game and UI
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void AHRBLexioPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		// Bind left mouse click via key binding
		FInputKeyBinding PressedBinding(FInputChord(EKeys::LeftMouseButton), IE_Pressed);
		PressedBinding.KeyDelegate.GetDelegateForManualSet().BindUObject(this, &AHRBLexioPlayerController::OnLeftMousePressed);
		InputComponent->KeyBindings.Add(PressedBinding);
	}
}

void AHRBLexioPlayerController::OnLeftMousePressed()
{
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		AHRBLexioHUD* LexioHUD = Cast<AHRBLexioHUD>(GetHUD());
		if (LexioHUD)
		{
			LexioHUD->HandleClick(FVector2D(MouseX, MouseY));
		}
	}
}

