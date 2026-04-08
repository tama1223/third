// Copyright HRB Lexio Project. All Rights Reserved.

#include "HRBLexioGameState.h"
#include "HRBLexioRuleEngine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HRBLexioGameState)

void UHRBLexioGameState::InitGame()
{
	// Create rule engine
	RuleEngine = NewObject<UHRBLexioRuleEngine>(this);

	// Reset scores and game round count
	for (int32 i = 0; i < NUM_PLAYERS; ++i)
	{
		PlayerScores[i] = 0;
	}
	GameRoundCount = 0;

	// Create and shuffle deck
	TArray<FHRBCardData> Deck = RuleEngine->CreateDeck();
	RuleEngine->ShuffleDeck(Deck);

	// Deal cards
	TArray<TArray<FHRBCardData>> DealtHands = RuleEngine->DealCards(Deck, NUM_PLAYERS);
	for (int32 i = 0; i < NUM_PLAYERS; ++i)
	{
		PlayerHands[i] = DealtHands[i];
	}

	// Sort all player hands by Lexio rank
	SortAllPlayerHands();

	// Find the player holding Cloud 3 — that player goes first
	int32 Cloud3Holder = FindCloud3Holder();
	CurrentPlayerIndex = (Cloud3Holder >= 0) ? Cloud3Holder : 0;

	// Reset state
	CurrentTableCombination = FHRBPlayedCombination();
	LastSubmitPlayerIndex = -1;
	ConsecutivePassCount = 0;
	bGameOver = false;
	WinnerIndex = -1;
	RoundNumber = 1;
	RoundHistory.Empty();
}

bool UHRBLexioGameState::SubmitCombination(int32 PlayerIndex, const TArray<FHRBCardData>& SelectedCards)
{
	if (bGameOver || PlayerIndex != CurrentPlayerIndex)
	{
		return false;
	}

	// Verify the player actually has these cards (check for duplicates)
	TArray<int32> UsedHandIndices;
	for (const FHRBCardData& Card : SelectedCards)
	{
		bool bFound = false;
		for (int32 HandIdx = 0; HandIdx < PlayerHands[PlayerIndex].Num(); ++HandIdx)
		{
			if (PlayerHands[PlayerIndex][HandIdx] == Card && !UsedHandIndices.Contains(HandIdx))
			{
				bFound = true;
				UsedHandIndices.Add(HandIdx);
				break;
			}
		}
		if (!bFound)
		{
			return false;
		}
	}

	// Create combination and validate
	FHRBPlayedCombination Attempt = RuleEngine->MakePlayedCombination(SelectedCards);
	if (!RuleEngine->CanPlayOnTable(Attempt, CurrentTableCombination))
	{
		return false;
	}

	// Remove cards from player's hand
	for (const FHRBCardData& Card : SelectedCards)
	{
		PlayerHands[PlayerIndex].RemoveAll([&Card](const FHRBCardData& HandCard)
		{
			return HandCard == Card;
		});
	}

	// Re-sort hand after card removal
	SortPlayerHand(PlayerIndex);

	// Update table state
	CurrentTableCombination = Attempt;
	LastSubmitPlayerIndex = PlayerIndex;
	ConsecutivePassCount = 0;

	// Add to round history
	RoundHistory.Add({ PlayerIndex, Attempt });

	// Check win condition
	if (CheckWinCondition(PlayerIndex))
	{
		return true;
	}

	// Advance to next player
	AdvanceTurn();

	return true;
}

bool UHRBLexioGameState::Pass(int32 PlayerIndex)
{
	if (bGameOver || PlayerIndex != CurrentPlayerIndex)
	{
		return false;
	}

	// Cannot pass if table is empty (must play something to start a round)
	if (!CurrentTableCombination.IsValid())
	{
		return false;
	}

	ConsecutivePassCount++;

	// If NUM_PLAYERS - 1 consecutive passes, round ends
	if (ConsecutivePassCount >= NUM_PLAYERS - 1)
	{
		StartNewRound();
		return true;
	}

	AdvanceTurn();

	return true;
}

const TArray<FHRBCardData>& UHRBLexioGameState::GetPlayerHand(int32 PlayerIndex) const
{
	check(PlayerIndex >= 0 && PlayerIndex < NUM_PLAYERS);
	return PlayerHands[PlayerIndex];
}

void UHRBLexioGameState::AdvanceTurn()
{
	CurrentPlayerIndex = (CurrentPlayerIndex + 1) % NUM_PLAYERS;
}

void UHRBLexioGameState::StartNewRound()
{
	CurrentTableCombination = FHRBPlayedCombination();
	ConsecutivePassCount = 0;
	RoundNumber++;
	RoundHistory.Empty();

	// Last submitter starts the new round
	if (LastSubmitPlayerIndex >= 0)
	{
		CurrentPlayerIndex = LastSubmitPlayerIndex;
	}
}

bool UHRBLexioGameState::CheckWinCondition(int32 PlayerIndex)
{
	if (PlayerHands[PlayerIndex].Num() == 0)
	{
		bGameOver = true;
		WinnerIndex = PlayerIndex;
		return true;
	}
	return false;
}

void UHRBLexioGameState::SortPlayerHand(int32 PlayerIndex)
{
	check(PlayerIndex >= 0 && PlayerIndex < NUM_PLAYERS);
	PlayerHands[PlayerIndex].Sort([](const FHRBCardData& A, const FHRBCardData& B)
	{
		if (A.GetRank() != B.GetRank())
		{
			return A.GetRank() < B.GetRank();
		}
		return A.GetSuitRank() < B.GetSuitRank();
	});
}

void UHRBLexioGameState::SortAllPlayerHands()
{
	for (int32 i = 0; i < NUM_PLAYERS; ++i)
	{
		SortPlayerHand(i);
	}
}

int32 UHRBLexioGameState::FindCloud3Holder() const
{
	for (int32 PlayerIdx = 0; PlayerIdx < NUM_PLAYERS; ++PlayerIdx)
	{
		for (const FHRBCardData& Card : PlayerHands[PlayerIdx])
		{
			if (Card.Number == 3 && Card.Suit == EHRBCardSuit::Cloud)
			{
				return PlayerIdx;
			}
		}
	}
	return -1;
}

int32 UHRBLexioGameState::GetPlayerScore(int32 PlayerIndex) const
{
	check(PlayerIndex >= 0 && PlayerIndex < NUM_PLAYERS);
	return PlayerScores[PlayerIndex];
}

void UHRBLexioGameState::GetAllScores(int32 OutScores[NUM_PLAYERS]) const
{
	for (int32 i = 0; i < NUM_PLAYERS; ++i)
	{
		OutScores[i] = PlayerScores[i];
	}
}

void UHRBLexioGameState::CalculateRoundScores(int32 OutDeltas[NUM_PLAYERS])
{
	// Calculate effective count for each player
	int32 EffectiveCounts[NUM_PLAYERS];
	for (int32 i = 0; i < NUM_PLAYERS; ++i)
	{
		int32 BaseCount = PlayerHands[i].Num();

		// Count the number of "2" tiles in hand
		int32 NumberOfTwos = 0;
		for (const FHRBCardData& Card : PlayerHands[i])
		{
			if (Card.Number == 2)
			{
				NumberOfTwos++;
			}
		}

		// Multiplier: 0 twos = x1, 1 two = x2, 2 twos = x3
		EffectiveCounts[i] = BaseCount * (NumberOfTwos + 1);
	}

	// For each player i: delta = sum over all j != i of (EffectiveCount[j] - EffectiveCount[i])
	for (int32 i = 0; i < NUM_PLAYERS; ++i)
	{
		int32 Delta = 0;
		for (int32 j = 0; j < NUM_PLAYERS; ++j)
		{
			if (i != j)
			{
				Delta += (EffectiveCounts[j] - EffectiveCounts[i]);
			}
		}
		OutDeltas[i] = Delta;
		PlayerScores[i] += Delta;
	}
}

bool UHRBLexioGameState::StartNewGameRound()
{
	GameRoundCount++;

	if (GameRoundCount > MAX_GAME_ROUNDS)
	{
		return false;
	}

	// Re-create deck, shuffle, deal (same as InitGame but don't reset scores)
	TArray<FHRBCardData> Deck = RuleEngine->CreateDeck();
	RuleEngine->ShuffleDeck(Deck);

	TArray<TArray<FHRBCardData>> DealtHands = RuleEngine->DealCards(Deck, NUM_PLAYERS);
	for (int32 i = 0; i < NUM_PLAYERS; ++i)
	{
		PlayerHands[i] = DealtHands[i];
	}

	SortAllPlayerHands();

	// Find Cloud 3 holder, set as current player
	int32 Cloud3Holder = FindCloud3Holder();
	CurrentPlayerIndex = (Cloud3Holder >= 0) ? Cloud3Holder : 0;

	// Reset round state
	CurrentTableCombination = FHRBPlayedCombination();
	LastSubmitPlayerIndex = -1;
	ConsecutivePassCount = 0;
	bGameOver = false;
	WinnerIndex = -1;
	RoundNumber = 1;
	RoundHistory.Empty();

	return true;
}
