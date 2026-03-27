// Copyright HRB Lexio Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Game/HRBLexioTypes.h"
#include "HRBLexioHUD.generated.h"

class UHRBLexioGameState;

/**
 * Canvas-based HUD for Lexio.
 * Draws all UI elements directly via DrawHUD() - no Blueprint dependency.
 */
UCLASS()
class AHRBLexioHUD : public AHUD
{
	GENERATED_BODY()

public:
	AHRBLexioHUD();

	virtual void DrawHUD() override;

	/** Called by PlayerController when left mouse button is clicked. */
	void HandleClick(const FVector2D& ClickPosition);

	/** Set the game state reference. */
	void SetGameState(UHRBLexioGameState* InGameState);

	/** Show a temporary status message. */
	void ShowStatusMessage(const FString& Message, float Duration = 2.0f);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	// --- Drawing Functions ---
	void DrawPlayerHand();
	void DrawTableCombination();
	void DrawAIInfo();
	void DrawButtons();
	void DrawStatusMessage();
	void DrawGameOverMessage();
	void DrawTurnInfo();
	void DrawRoundInfo();

	// --- Hit Test Helpers ---
	FVector2D GetCardPosition(int32 Index, int32 TotalCards) const;
	FVector2D GetCardSize() const;
	bool IsPointInRect(const FVector2D& Point, const FVector2D& RectPos, const FVector2D& RectSize) const;

	// --- Button Rects (calculated each frame) ---
	FVector2D SubmitButtonPos;
	FVector2D SubmitButtonSize;
	FVector2D PassButtonPos;
	FVector2D PassButtonSize;

	// --- State ---
	UPROPERTY()
	TObjectPtr<UHRBLexioGameState> GameState;

	/** Indices into the human player's hand that are currently selected. */
	TArray<int32> SelectedCardIndices;

	/** Current status message text. */
	FString CurrentStatusMessage;

	/** Remaining time for the status message. */
	float StatusMessageTimer = 0.0f;

	/** Card visual dimensions. */
	static constexpr float CardWidth = 60.0f;
	static constexpr float CardHeight = 80.0f;
	static constexpr float CardSpacing = 8.0f;
	static constexpr float SelectedOffsetY = -15.0f;

	static constexpr float TableCardWidth = 70.0f;
	static constexpr float TableCardHeight = 90.0f;

	static constexpr float ButtonWidth = 100.0f;
	static constexpr float ButtonHeight = 40.0f;
	static constexpr float ButtonSpacing = 20.0f;

	static constexpr int32 HumanPlayerIndex = 0;

	/** Font for drawing text. */
	UPROPERTY()
	TObjectPtr<UFont> HUDFont;

	/** Cached viewport size (updated each DrawHUD frame). */
	FVector2D CachedViewportSize = FVector2D::ZeroVector;
};
