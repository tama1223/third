// Copyright HRB Lexio Project. All Rights Reserved.

#include "HRBLexioGameState.h"
#include "HRBLexioRuleEngine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HRBLexioGameState)

void UHRBLexioGameState::InitGame()
{
	// Create rule engine
	RuleEngine = NewObject<UHRBLexioRuleEngine>(this);

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

	// Random first player
	CurrentPlayerIndex = FMath::RandRange(0, NUM_PLAYERS - 1);

	// Reset state
	CurrentTableCombination = FHRBPlayedCombination();
	LastSubmitPlayerIndex = -1;
	ConsecutivePassCount = 0;
	bGameOver = false;
	WinnerIndex = -1;
	RoundNumber = 1;
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
		return A.InstanceId < B.InstanceId;
	});
}

void UHRBLexioGameState::SortAllPlayerHands()
{
	for (int32 i = 0; i < NUM_PLAYERS; ++i)
	{
		SortPlayerHand(i);
	}
}
