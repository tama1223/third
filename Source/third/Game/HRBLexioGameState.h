// Copyright HRB Lexio Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HRBLexioTypes.h"
#include "HRBLexioGameState.generated.h"

class UHRBLexioRuleEngine;

/**
 * Manages the game flow: turn order, submissions, passes, round/game end.
 * Pure logic, no UI dependency.
 */
UCLASS()
class UHRBLexioGameState : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 NUM_PLAYERS = 3;

	/** Initialize the game: create deck, shuffle, deal, pick random first player. */
	void InitGame();

	/**
	 * Current player submits a combination.
	 * @return true if the submission was valid and accepted.
	 */
	bool SubmitCombination(int32 PlayerIndex, const TArray<FHRBCardData>& SelectedCards);

	/**
	 * Current player passes.
	 * @return true if the pass was valid.
	 */
	bool Pass(int32 PlayerIndex);

	/** Get the current player index. */
	int32 GetCurrentPlayerIndex() const { return CurrentPlayerIndex; }

	/** Check if the game is over. */
	bool IsGameOver() const { return bGameOver; }

	/** Get the winner index (-1 if no winner yet). */
	int32 GetWinnerIndex() const { return WinnerIndex; }

	/** Get a player's hand. */
	const TArray<FHRBCardData>& GetPlayerHand(int32 PlayerIndex) const;

	/** Get the current table combination. */
	const FHRBPlayedCombination& GetCurrentTableCombination() const { return CurrentTableCombination; }

	/** Get the consecutive pass count. */
	int32 GetConsecutivePassCount() const { return ConsecutivePassCount; }

	/** Get the last submit player index. */
	int32 GetLastSubmitPlayerIndex() const { return LastSubmitPlayerIndex; }

	/** Check if a new round just started (table is empty). */
	bool IsNewRound() const { return !CurrentTableCombination.IsValid(); }

	/** Get the rule engine. */
	UHRBLexioRuleEngine* GetRuleEngine() const { return RuleEngine; }

	/** Get the current round number (1-based). */
	int32 GetRoundNumber() const { return RoundNumber; }

	/** Round history entry: who played what. */
	struct FRoundHistoryEntry
	{
		int32 PlayerIndex;
		FHRBPlayedCombination Combination;
	};

	/** Get the current round's submission history. */
	const TArray<FRoundHistoryEntry>& GetRoundHistory() const { return RoundHistory; }

private:
	/** Sort a player's hand by Lexio rank (3,4,5,6,7,8,9,1,2). */
	void SortPlayerHand(int32 PlayerIndex);

	/** Sort all players' hands. */
	void SortAllPlayerHands();

	/** Advance to the next player's turn. */
	void AdvanceTurn();

	/** Start a new round: clear table, last submitter leads. */
	void StartNewRound();

	/** Check if a player has won (empty hand). */
	bool CheckWinCondition(int32 PlayerIndex);

	TArray<FHRBCardData> PlayerHands[NUM_PLAYERS];

	UPROPERTY()
	int32 CurrentPlayerIndex = 0;

	UPROPERTY()
	FHRBPlayedCombination CurrentTableCombination;

	UPROPERTY()
	int32 LastSubmitPlayerIndex = -1;

	UPROPERTY()
	int32 ConsecutivePassCount = 0;

	UPROPERTY()
	bool bGameOver = false;

	UPROPERTY()
	int32 WinnerIndex = -1;

	UPROPERTY()
	int32 RoundNumber = 1;

	UPROPERTY()
	TObjectPtr<UHRBLexioRuleEngine> RuleEngine;

	/** History of submissions in the current round. Cleared on new round. */
	TArray<FRoundHistoryEntry> RoundHistory;
};
