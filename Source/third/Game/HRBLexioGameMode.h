// Copyright HRB Lexio Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HRBLexioGameMode.generated.h"

class UHRBLexioGameState;
struct FHRBCardData;

/**
 * GameMode for Lexio.
 * Player 0 = Human (UI input), Player 1,2 = AI (auto-play).
 */
UCLASS()
class AHRBLexioGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHRBLexioGameMode();

	/**
	 * Called by HUD when the human player submits selected cards.
	 * @return true if the submission was valid.
	 */
	bool ProcessHumanTurn(const TArray<FHRBCardData>& SelectedCards);

	/** Called by HUD when the human player passes. */
	void ProcessHumanPass();

	/** Get the game state. */
	UHRBLexioGameState* GetLexioGameState() const { return LexioGameState; }

protected:
	virtual void BeginPlay() override;

private:
	/** Initialize and start the game. */
	void StartGame();

	/** Process an AI player's turn. */
	void ProcessAITurn();

	/** Called after any turn to check if the next player is AI and schedule their turn. */
	void OnTurnAdvanced();

	/** Timer callback for AI turn delay. */
	void OnAITurnTimerFired();

	UPROPERTY()
	TObjectPtr<UHRBLexioGameState> LexioGameState;

	/** Delay before AI plays (seconds). */
	float AITurnDelay = 1.0f;

	/** Timer handle for AI turn delay. */
	FTimerHandle AITurnTimerHandle;

	static constexpr int32 HumanPlayerIndex = 0;
};
