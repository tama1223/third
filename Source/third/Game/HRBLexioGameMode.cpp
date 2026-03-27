// Copyright HRB Lexio Project. All Rights Reserved.

#include "HRBLexioGameMode.h"
#include "HRBLexioGameState.h"
#include "HRBLexioRuleEngine.h"
#include "HRBLexioTypes.h"
#include "HRBLexioPlayerController.h"
#include "UI/HRBLexioHUD.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HRBLexioGameMode)

DEFINE_LOG_CATEGORY_STATIC(LogHRBLexio, Log, All);

AHRBLexioGameMode::AHRBLexioGameMode()
{
	// Set default classes - no Blueprint dependency
	PlayerControllerClass = AHRBLexioPlayerController::StaticClass();
	HUDClass = AHRBLexioHUD::StaticClass();
}

void AHRBLexioGameMode::BeginPlay()
{
	Super::BeginPlay();

	StartGame();
}

void AHRBLexioGameMode::StartGame()
{
	// Create and initialize game state
	LexioGameState = NewObject<UHRBLexioGameState>(this);
	LexioGameState->InitGame();

	UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] === Game Start ==="));

	// Log initial hands
	for (int32 i = 0; i < UHRBLexioGameState::NUM_PLAYERS; ++i)
	{
		const TArray<FHRBCardData>& Hand = LexioGameState->GetPlayerHand(i);
		FString HandStr = TEXT("[");
		for (int32 j = 0; j < Hand.Num(); ++j)
		{
			if (j > 0) HandStr += TEXT(",");
			HandStr += Hand[j].ToString();
		}
		HandStr += TEXT("]");
		UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Player %d Hand: %s"), i, *HandStr);
	}

	UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] First player: Player %d"), LexioGameState->GetCurrentPlayerIndex());

	// Pass GameState to HUD
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		AHRBLexioHUD* HUD = Cast<AHRBLexioHUD>(PC->GetHUD());
		if (HUD)
		{
			HUD->SetGameState(LexioGameState);
		}
	}

	// If AI goes first, start their turn with delay
	OnTurnAdvanced();
}

bool AHRBLexioGameMode::ProcessHumanTurn(const TArray<FHRBCardData>& SelectedCards)
{
	if (!LexioGameState || LexioGameState->IsGameOver())
	{
		return false;
	}

	if (LexioGameState->GetCurrentPlayerIndex() != HumanPlayerIndex)
	{
		return false;
	}

	bool bSuccess = LexioGameState->SubmitCombination(HumanPlayerIndex, SelectedCards);
	if (bSuccess)
	{
		UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Human: Submit %d card(s)"), SelectedCards.Num());

		if (LexioGameState->IsGameOver())
		{
			UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] === Game Over === Winner: Player %d"), LexioGameState->GetWinnerIndex());
			return true;
		}

		OnTurnAdvanced();
	}

	return bSuccess;
}

void AHRBLexioGameMode::ProcessHumanPass()
{
	if (!LexioGameState || LexioGameState->IsGameOver())
	{
		return;
	}

	if (LexioGameState->GetCurrentPlayerIndex() != HumanPlayerIndex)
	{
		return;
	}

	bool bSuccess = LexioGameState->Pass(HumanPlayerIndex);
	if (bSuccess)
	{
		UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] Human: Pass"));
		OnTurnAdvanced();
	}
}

void AHRBLexioGameMode::OnTurnAdvanced()
{
	if (LexioGameState->IsGameOver())
	{
		return;
	}

	const int32 CurrentPlayer = LexioGameState->GetCurrentPlayerIndex();
	if (CurrentPlayer != HumanPlayerIndex)
	{
		// Schedule AI turn with delay
		GetWorld()->GetTimerManager().SetTimer(
			AITurnTimerHandle,
			this,
			&AHRBLexioGameMode::OnAITurnTimerFired,
			AITurnDelay,
			false);
	}
	// If human turn, just wait for UI input
}

void AHRBLexioGameMode::OnAITurnTimerFired()
{
	ProcessAITurn();
}

void AHRBLexioGameMode::ProcessAITurn()
{
	if (!LexioGameState || LexioGameState->IsGameOver())
	{
		return;
	}

	const int32 CurrentPlayer = LexioGameState->GetCurrentPlayerIndex();
	if (CurrentPlayer == HumanPlayerIndex)
	{
		return; // Not AI's turn
	}

	const TArray<FHRBCardData>& Hand = LexioGameState->GetPlayerHand(CurrentPlayer);
	UHRBLexioRuleEngine* RuleEngine = LexioGameState->GetRuleEngine();

	if (Hand.Num() == 0)
	{
		return;
	}

	const FHRBPlayedCombination& TableTop = LexioGameState->GetCurrentTableCombination();
	const bool bTableEmpty = !TableTop.IsValid();

	if (bTableEmpty)
	{
		// New round: play the lowest combination
		TArray<FHRBPlayedCombination> AllCombinations = RuleEngine->FindAllValidCombinations(Hand);
		if (AllCombinations.Num() > 0)
		{
			TArray<FHRBCardData> CardsToPlay = AllCombinations[0].Cards;
			bool bSuccess = LexioGameState->SubmitCombination(CurrentPlayer, CardsToPlay);
			if (bSuccess)
			{
				UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] AI %d: Submit %s"), CurrentPlayer, *AllCombinations[0].ToString());
			}
		}
	}
	else
	{
		// Try to beat the table
		TArray<FHRBPlayedCombination> ValidCombinations = RuleEngine->FindAllValidCombinations(Hand, TableTop.Type);

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
			TArray<FHRBCardData> CardsToPlay = PlayableCombinations[0].Cards;
			bool bSuccess = LexioGameState->SubmitCombination(CurrentPlayer, CardsToPlay);
			if (bSuccess)
			{
				UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] AI %d: Submit %s"), CurrentPlayer, *PlayableCombinations[0].ToString());
			}
		}
		else
		{
			// Pass
			bool bSuccess = LexioGameState->Pass(CurrentPlayer);
			if (bSuccess)
			{
				UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] AI %d: Pass"), CurrentPlayer);
			}
		}
	}

	if (LexioGameState->IsGameOver())
	{
		UE_LOG(LogHRBLexio, Log, TEXT("[HRBLexio] === Game Over === Winner: Player %d"), LexioGameState->GetWinnerIndex());
		return;
	}

	// Continue to next turn
	OnTurnAdvanced();
}
