// Copyright HRB Lexio Project. All Rights Reserved.

#include "HRBLexioGameMode.h"
#include "HRBLexioGameState.h"
#include "HRBLexioRuleEngine.h"
#include "HRBLexioTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HRBLexioGameMode)

DEFINE_LOG_CATEGORY_STATIC(LogHRBLexio, Log, All);

AHRBLexioGameMode::AHRBLexioGameMode()
{
}

void AHRBLexioGameMode::BeginPlay()
{
	Super::BeginPlay();

	RunSimulation();
}

void AHRBLexioGameMode::LogPlayerHand(int32 PlayerIndex, const TArray<FHRBCardData>& Hand) const
{
	FString HandStr = TEXT("[");
	for (int32 i = 0; i < Hand.Num(); ++i)
	{
		if (i > 0)
		{
			HandStr += TEXT(",");
		}
		HandStr += Hand[i].ToString();
	}
	HandStr += TEXT("]");

	UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Player %d Hand: %s"), PlayerIndex, *HandStr);
}

void AHRBLexioGameMode::RunSimulation()
{
	// Create and initialize game state
	LexioGameState = NewObject<UHRBLexioGameState>(this);
	LexioGameState->InitGame();

	UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] === Game Start ==="));

	// Log initial hands
	for (int32 i = 0; i < UHRBLexioGameState::NUM_PLAYERS; ++i)
	{
		LogPlayerHand(i, LexioGameState->GetPlayerHand(i));
	}

	UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] First player: Player %d"), LexioGameState->GetCurrentPlayerIndex());

	UHRBLexioRuleEngine* RuleEngine = LexioGameState->GetRuleEngine();
	int32 RoundNumber = 1;
	int32 TurnCount = 0;
	const int32 MaxTurns = 500; // Safety limit

	UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] --- Round %d ---"), RoundNumber);

	bool bPrevWasNewRound = false;

	while (!LexioGameState->IsGameOver() && TurnCount < MaxTurns)
	{
		TurnCount++;
		const int32 CurrentPlayer = LexioGameState->GetCurrentPlayerIndex();
		const TArray<FHRBCardData>& Hand = LexioGameState->GetPlayerHand(CurrentPlayer);

		if (Hand.Num() == 0)
		{
			// Should not happen, but safety check
			break;
		}

		// Determine what type we need to match
		const FHRBPlayedCombination& TableTop = LexioGameState->GetCurrentTableCombination();
		const bool bTableEmpty = !TableTop.IsValid();

		if (bTableEmpty)
		{
			// New round: find all combinations and play the lowest
			TArray<FHRBPlayedCombination> AllCombinations = RuleEngine->FindAllValidCombinations(Hand);

			if (AllCombinations.Num() > 0)
			{
				// Pick the lowest combination (first in sorted list)
				const FHRBPlayedCombination& ToPlay = AllCombinations[0];

				// Extract card data for submission
				TArray<FHRBCardData> CardsToPlay = ToPlay.Cards;

				bool bOldIsNewRound = LexioGameState->IsNewRound();
				bool bSuccess = LexioGameState->SubmitCombination(CurrentPlayer, CardsToPlay);

				if (bSuccess)
				{
					UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Player %d: Submit %s"), CurrentPlayer, *ToPlay.ToString());

					if (LexioGameState->IsGameOver())
					{
						UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Player %d: Hand empty! Winner: Player %d"),
							CurrentPlayer, LexioGameState->GetWinnerIndex());
						break;
					}
				}
			}
		}
		else
		{
			// Try to play a combination that beats the table
			TArray<FHRBPlayedCombination> ValidCombinations = RuleEngine->FindAllValidCombinations(Hand, TableTop.Type);

			// Filter to only those that can beat the table
			TArray<FHRBPlayedCombination> PlayableCombinations;
			for (const FHRBPlayedCombination& Combo : ValidCombinations)
			{
				if (Combo.CanBeatOther(TableTop))
				{
					PlayableCombinations.Add(Combo);
				}
			}

			if (PlayableCombinations.Num() > 0)
			{
				// Play the lowest valid combination
				const FHRBPlayedCombination& ToPlay = PlayableCombinations[0];
				TArray<FHRBCardData> CardsToPlay = ToPlay.Cards;

				bool bSuccess = LexioGameState->SubmitCombination(CurrentPlayer, CardsToPlay);
				if (bSuccess)
				{
					UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Player %d: Submit %s"), CurrentPlayer, *ToPlay.ToString());

					if (LexioGameState->IsGameOver())
					{
						UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Player %d: Hand empty! Winner: Player %d"),
							CurrentPlayer, LexioGameState->GetWinnerIndex());
						break;
					}
				}
			}
			else
			{
				// Pass
				int32 PassCountBefore = LexioGameState->GetConsecutivePassCount();
				bool bSuccess = LexioGameState->Pass(CurrentPlayer);
				if (bSuccess)
				{
					// Check if round ended (pass count reset means new round started)
					if (LexioGameState->GetConsecutivePassCount() == 0)
					{
						UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Player %d: Pass (%d consecutive passes -> Round End)"),
							CurrentPlayer, PassCountBefore + 1);

						RoundNumber++;
						UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] --- Round %d (Leader: Player %d) ---"),
							RoundNumber, LexioGameState->GetCurrentPlayerIndex());
					}
					else
					{
						UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Player %d: Pass"), CurrentPlayer);
					}
				}
			}
		}
	}

	if (TurnCount >= MaxTurns)
	{
		UE_LOG(LogHRBLexio, Warning, TEXT("[HRBLexio] Simulation reached max turn limit (%d). Possible infinite loop."), MaxTurns);
	}

	UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] === Game Over ==="));
}
