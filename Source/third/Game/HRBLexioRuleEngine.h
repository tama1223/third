// Copyright HRB Lexio Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HRBLexioTypes.h"
#include "HRBLexioRuleEngine.generated.h"

/**
 * Pure game logic engine for Lexio. No UI dependencies.
 * Handles deck management, combination validation, and rule checking.
 */
UCLASS()
class UHRBLexioRuleEngine : public UObject
{
	GENERATED_BODY()

public:
	/** Creates a standard 36-card deck (numbers 1~9, 4 suits each). */
	TArray<FHRBCardData> CreateDeck() const;

	/** Shuffles the given deck in place. */
	void ShuffleDeck(TArray<FHRBCardData>& Deck) const;

	/** Deals cards from the deck to NumPlayers players (12 cards each for 3 players). */
	TArray<TArray<FHRBCardData>> DealCards(const TArray<FHRBCardData>& Deck, int32 NumPlayers) const;

	/** Determines the combination type of the given set of cards. */
	EHRBCardCombinationType DetermineCombinationType(const TArray<FHRBCardData>& Cards) const;

	/** Returns true if the given cards form a valid combination. */
	bool IsValidCombination(const TArray<FHRBCardData>& Cards) const;

	/** Returns true if the Attempt can be played on top of the current TableTop. */
	bool CanPlayOnTable(const FHRBPlayedCombination& Attempt, const FHRBPlayedCombination& TableTop) const;

	/** Creates a FHRBPlayedCombination from the given cards. Returns invalid combination if cards are not valid. */
	FHRBPlayedCombination MakePlayedCombination(const TArray<FHRBCardData>& Cards) const;

	/**
	 * Finds all valid combinations from the given hand.
	 * If RequiredType is None, returns all Single/Pair/Triple/5-card combinations.
	 * If RequiredType is specified, returns only that type (or all 5-card types if a 5-card type is specified).
	 */
	TArray<FHRBPlayedCombination> FindAllValidCombinations(
		const TArray<FHRBCardData>& Hand,
		EHRBCardCombinationType RequiredType = EHRBCardCombinationType::None) const;

	/** Returns true if the type is a 5-card combination. */
	static bool IsFiveCardType(EHRBCardCombinationType Type);
};
