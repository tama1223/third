// Copyright HRB Lexio Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HRBLexioPlayerController.generated.h"

/**
 * Player controller for Lexio.
 * Enables mouse cursor and forwards click events to the HUD.
 */
UCLASS()
class AHRBLexioPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHRBLexioPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	/** Handle left mouse button press. */
	void OnLeftMousePressed();
};
