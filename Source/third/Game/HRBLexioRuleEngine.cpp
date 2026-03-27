// Copyright HRB Lexio Project. All Rights Reserved.

#include "HRBLexioRuleEngine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HRBLexioRuleEngine)

TArray<FHRBCardData> UHRBLexioRuleEngine::CreateDeck() const
{
	TArray<FHRBCardData> Deck;
	Deck.Reserve(36);

	int32 InstanceId = 0;
	for (int32 Number = 1; Number <= 9; ++Number)
	{
		for (int32 Copy = 0; Copy < 4; ++Copy)
		{
			FHRBCardData Card;
			Card.Number = Number;
			Card.InstanceId = InstanceId++;
			Deck.Add(Card);
		}
	}

	return Deck;
}

void UHRBLexioRuleEngine::ShuffleDeck(TArray<FHRBCardData>& Deck) const
{
	const int32 NumCards = Deck.Num();
	for (int32 i = NumCards - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Deck.Swap(i, j);
	}
}

TArray<TArray<FHRBCardData>> UHRBLexioRuleEngine::DealCards(const TArray<FHRBCardData>& Deck, int32 NumPlayers) const
{
	TArray<TArray<FHRBCardData>> Hands;
	Hands.SetNum(NumPlayers);

	const int32 CardsPerPlayer = Deck.Num() / NumPlayers;

	for (int32 PlayerIdx = 0; PlayerIdx < NumPlayers; ++PlayerIdx)
	{
		const int32 StartIdx = PlayerIdx * CardsPerPlayer;
		for (int32 CardIdx = 0; CardIdx < CardsPerPlayer; ++CardIdx)
		{
			Hands[PlayerIdx].Add(Deck[StartIdx + CardIdx]);
		}

		// Sort each hand by number for readability
		Hands[PlayerIdx].Sort([](const FHRBCardData& A, const FHRBCardData& B)
		{
			return A.Number < B.Number;
		});
	}

	return Hands;
}

EHRBCardCombinationType UHRBLexioRuleEngine::DetermineCombinationType(const TArray<FHRBCardData>& Cards) const
{
	if (Cards.Num() == 0)
	{
		return EHRBCardCombinationType::None;
	}

	if (Cards.Num() == 1)
	{
		return EHRBCardCombinationType::Single;
	}

	// Check if all cards have the same number
	const int32 FirstNumber = Cards[0].Number;
	bool bAllSameNumber = true;
	for (int32 i = 1; i < Cards.Num(); ++i)
	{
		if (Cards[i].Number != FirstNumber)
		{
			bAllSameNumber = false;
			break;
		}
	}

	if (!bAllSameNumber)
	{
		return EHRBCardCombinationType::None;
	}

	if (Cards.Num() == 2)
	{
		return EHRBCardCombinationType::Pair;
	}

	if (Cards.Num() == 3)
	{
		return EHRBCardCombinationType::Triple;
	}

	return EHRBCardCombinationType::None;
}

bool UHRBLexioRuleEngine::IsValidCombination(const TArray<FHRBCardData>& Cards) const
{
	return DetermineCombinationType(Cards) != EHRBCardCombinationType::None;
}

bool UHRBLexioRuleEngine::CanPlayOnTable(const FHRBPlayedCombination& Attempt, const FHRBPlayedCombination& TableTop) const
{
	// If table is empty (no current combination), any valid combination can be played
	if (!TableTop.IsValid())
	{
		return Attempt.IsValid();
	}

	// Must be a valid combination that can beat the current table top
	return Attempt.IsValid() && Attempt.CanBeatOther(TableTop);
}

FHRBPlayedCombination UHRBLexioRuleEngine::MakePlayedCombination(const TArray<FHRBCardData>& Cards) const
{
	FHRBPlayedCombination Combination;
	Combination.Type = DetermineCombinationType(Cards);
	Combination.Cards = Cards;

	if (Combination.Type != EHRBCardCombinationType::None)
	{
		// RankValue is the card number (all cards in a valid combination have the same number,
		// or it's a single card)
		int32 MaxNumber = 0;
		for (const FHRBCardData& Card : Cards)
		{
			MaxNumber = FMath::Max(MaxNumber, Card.Number);
		}
		Combination.RankValue = MaxNumber;
	}

	return Combination;
}

TArray<FHRBPlayedCombination> UHRBLexioRuleEngine::FindAllValidCombinations(
	const TArray<FHRBCardData>& Hand,
	EHRBCardCombinationType RequiredType) const
{
	TArray<FHRBPlayedCombination> Results;

	// Group cards by number
	TMap<int32, TArray<FHRBCardData>> CardsByNumber;
	for (const FHRBCardData& Card : Hand)
	{
		CardsByNumber.FindOrAdd(Card.Number).Add(Card);
	}

	for (const auto& Pair : CardsByNumber)
	{
		const int32 Number = Pair.Key;
		const TArray<FHRBCardData>& CardsOfNumber = Pair.Value;
		const int32 Count = CardsOfNumber.Num();

		// Singles
		if (RequiredType == EHRBCardCombinationType::None || RequiredType == EHRBCardCombinationType::Single)
		{
			for (int32 i = 0; i < Count; ++i)
			{
				TArray<FHRBCardData> SingleCards;
				SingleCards.Add(CardsOfNumber[i]);
				Results.Add(MakePlayedCombination(SingleCards));
			}
		}

		// Pairs
		if ((RequiredType == EHRBCardCombinationType::None || RequiredType == EHRBCardCombinationType::Pair) && Count >= 2)
		{
			for (int32 i = 0; i < Count - 1; ++i)
			{
				for (int32 j = i + 1; j < Count; ++j)
				{
					TArray<FHRBCardData> PairCards;
					PairCards.Add(CardsOfNumber[i]);
					PairCards.Add(CardsOfNumber[j]);
					Results.Add(MakePlayedCombination(PairCards));
				}
			}
		}

		// Triples
		if ((RequiredType == EHRBCardCombinationType::None || RequiredType == EHRBCardCombinationType::Triple) && Count >= 3)
		{
			for (int32 i = 0; i < Count - 2; ++i)
			{
				for (int32 j = i + 1; j < Count - 1; ++j)
				{
					for (int32 k = j + 1; k < Count; ++k)
					{
						TArray<FHRBCardData> TripleCards;
						TripleCards.Add(CardsOfNumber[i]);
						TripleCards.Add(CardsOfNumber[j]);
						TripleCards.Add(CardsOfNumber[k]);
						Results.Add(MakePlayedCombination(TripleCards));
					}
				}
			}
		}
	}

	// Sort by rank value for consistent ordering
	Results.Sort([](const FHRBPlayedCombination& A, const FHRBPlayedCombination& B)
	{
		if (A.Type != B.Type)
		{
			return static_cast<uint8>(A.Type) < static_cast<uint8>(B.Type);
		}
		return A.RankValue < B.RankValue;
	});

	return Results;
}
