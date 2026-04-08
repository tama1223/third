// Copyright HRB Lexio Project. All Rights Reserved.

#include "HRBLexioRuleEngine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HRBLexioRuleEngine)

bool UHRBLexioRuleEngine::IsFiveCardType(EHRBCardCombinationType Type)
{
	return Type == EHRBCardCombinationType::Straight
		|| Type == EHRBCardCombinationType::Flush
		|| Type == EHRBCardCombinationType::FullHouse
		|| Type == EHRBCardCombinationType::FourOfAKind
		|| Type == EHRBCardCombinationType::StraightFlush;
}

TArray<FHRBCardData> UHRBLexioRuleEngine::CreateDeck() const
{
	TArray<FHRBCardData> Deck;
	Deck.Reserve(36);

	int32 InstanceId = 0;
	for (int32 SuitIdx = 0; SuitIdx < 4; ++SuitIdx)
	{
		for (int32 Number = 1; Number <= 9; ++Number)
		{
			FHRBCardData Card;
			Card.Number = Number;
			Card.Suit = static_cast<EHRBCardSuit>(SuitIdx);
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

		// Sort each hand by rank first, then suit rank (Lexio order: 3 < 4 < ... < 9 < 1 < 2, then Cloud < Star < Moon < Sun)
		Hands[PlayerIdx].Sort([](const FHRBCardData& A, const FHRBCardData& B)
		{
			if (A.GetRank() != B.GetRank())
			{
				return A.GetRank() < B.GetRank();
			}
			return A.GetSuitRank() < B.GetSuitRank();
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

	// For 2 and 3 cards: check if all have the same number
	if (Cards.Num() == 2 || Cards.Num() == 3)
	{
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

		return EHRBCardCombinationType::Triple;
	}

	// 5-card combinations
	if (Cards.Num() == 5)
	{
		// Collect ranks and check suits
		TArray<int32> Ranks;
		Ranks.Reserve(5);
		bool bAllSameSuit = true;
		const EHRBCardSuit FirstSuit = Cards[0].Suit;

		for (int32 i = 0; i < 5; ++i)
		{
			Ranks.Add(Cards[i].GetRank());
			if (Cards[i].Suit != FirstSuit)
			{
				bAllSameSuit = false;
			}
		}

		// Sort ranks
		Ranks.Sort();

		// Check for straight: 5 consecutive ranks
		bool bIsStraight = true;
		for (int32 i = 1; i < 5; ++i)
		{
			if (Ranks[i] != Ranks[i - 1] + 1)
			{
				bIsStraight = false;
				break;
			}
		}

		// StraightFlush: both straight and flush
		if (bIsStraight && bAllSameSuit)
		{
			return EHRBCardCombinationType::StraightFlush;
		}

		// Group by number to detect FourOfAKind/FullHouse
		int32 GroupCounts[9]; // index 0-8 for numbers 1-9
		for (int32 i = 0; i < 9; ++i)
		{
			GroupCounts[i] = 0;
		}
		for (const FHRBCardData& Card : Cards)
		{
			GroupCounts[Card.Number - 1]++;
		}

		bool bHasFour = false;
		bool bHasThree = false;
		bool bHasTwo = false;
		for (int32 i = 0; i < 9; ++i)
		{
			if (GroupCounts[i] == 4) bHasFour = true;
			if (GroupCounts[i] == 3) bHasThree = true;
			if (GroupCounts[i] == 2) bHasTwo = true;
		}

		// FourOfAKind: 4 of one number + 1 any
		if (bHasFour)
		{
			return EHRBCardCombinationType::FourOfAKind;
		}

		// FullHouse: 3 of one number + 2 of another
		if (bHasThree && bHasTwo)
		{
			return EHRBCardCombinationType::FullHouse;
		}

		// Flush: all same suit (but not a straight, already checked above)
		if (bAllSameSuit)
		{
			return EHRBCardCombinationType::Flush;
		}

		// Straight: consecutive ranks (but not all same suit, already checked above)
		if (bIsStraight)
		{
			return EHRBCardCombinationType::Straight;
		}
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

	if (!Attempt.IsValid())
	{
		return false;
	}

	const bool bTableIsFiveCard = IsFiveCardType(TableTop.Type);
	const bool bAttemptIsFiveCard = IsFiveCardType(Attempt.Type);

	// If table has a 5-card combo, attempt must also be 5-card
	if (bTableIsFiveCard && !bAttemptIsFiveCard)
	{
		return false;
	}

	// If table has 1/2/3-card combo, attempt cannot be 5-card
	if (!bTableIsFiveCard && bAttemptIsFiveCard)
	{
		return false;
	}

	// Use CanBeatOther for the actual comparison
	return Attempt.CanBeatOther(TableTop);
}

FHRBPlayedCombination UHRBLexioRuleEngine::MakePlayedCombination(const TArray<FHRBCardData>& Cards) const
{
	FHRBPlayedCombination Combination;
	Combination.Type = DetermineCombinationType(Cards);
	Combination.Cards = Cards;

	if (Combination.Type == EHRBCardCombinationType::None)
	{
		return Combination;
	}

	switch (Combination.Type)
	{
	case EHRBCardCombinationType::Single:
	{
		Combination.RankValue = Cards[0].GetRank();
		Combination.SubRankValue = Cards[0].GetSuitRank();
		break;
	}
	case EHRBCardCombinationType::Pair:
	{
		Combination.RankValue = Cards[0].GetRank(); // same number so same rank
		int32 MaxSuitRank = -1;
		for (const FHRBCardData& Card : Cards)
		{
			if (Card.GetSuitRank() > MaxSuitRank)
			{
				MaxSuitRank = Card.GetSuitRank();
			}
		}
		Combination.SubRankValue = MaxSuitRank;
		break;
	}
	case EHRBCardCombinationType::Triple:
	{
		Combination.RankValue = Cards[0].GetRank(); // same number so same rank
		int32 MaxSuitRank = -1;
		for (const FHRBCardData& Card : Cards)
		{
			if (Card.GetSuitRank() > MaxSuitRank)
			{
				MaxSuitRank = Card.GetSuitRank();
			}
		}
		Combination.SubRankValue = MaxSuitRank;
		break;
	}
	case EHRBCardCombinationType::Straight:
	{
		// Highest card's rank and suit rank
		int32 MaxRank = -1;
		int32 BestSuitRank = -1;
		for (const FHRBCardData& Card : Cards)
		{
			if (Card.GetRank() > MaxRank)
			{
				MaxRank = Card.GetRank();
				BestSuitRank = Card.GetSuitRank();
			}
			else if (Card.GetRank() == MaxRank && Card.GetSuitRank() > BestSuitRank)
			{
				BestSuitRank = Card.GetSuitRank();
			}
		}
		Combination.RankValue = MaxRank;
		Combination.SubRankValue = BestSuitRank;
		break;
	}
	case EHRBCardCombinationType::Flush:
	{
		// Highest card's rank (Lexio rank), SubRankValue = suit rank (all same suit)
		int32 MaxRank = -1;
		for (const FHRBCardData& Card : Cards)
		{
			if (Card.GetRank() > MaxRank)
			{
				MaxRank = Card.GetRank();
			}
		}
		Combination.RankValue = MaxRank;
		Combination.SubRankValue = Cards[0].GetSuitRank(); // all same suit
		break;
	}
	case EHRBCardCombinationType::FullHouse:
	{
		// RankValue = rank of the triple's number
		// Find which number has 3 cards
		int32 GroupCounts[9];
		for (int32 i = 0; i < 9; ++i) GroupCounts[i] = 0;
		for (const FHRBCardData& Card : Cards)
		{
			GroupCounts[Card.Number - 1]++;
		}
		for (int32 i = 0; i < 9; ++i)
		{
			if (GroupCounts[i] == 3)
			{
				// Convert number (i+1) to rank
				FHRBCardData TempCard;
				TempCard.Number = i + 1;
				Combination.RankValue = TempCard.GetRank();
				break;
			}
		}
		Combination.SubRankValue = 0;
		break;
	}
	case EHRBCardCombinationType::FourOfAKind:
	{
		// RankValue = rank of the quad's number
		int32 GroupCounts[9];
		for (int32 i = 0; i < 9; ++i) GroupCounts[i] = 0;
		for (const FHRBCardData& Card : Cards)
		{
			GroupCounts[Card.Number - 1]++;
		}
		for (int32 i = 0; i < 9; ++i)
		{
			if (GroupCounts[i] == 4)
			{
				FHRBCardData TempCard;
				TempCard.Number = i + 1;
				Combination.RankValue = TempCard.GetRank();
				break;
			}
		}
		Combination.SubRankValue = 0;
		break;
	}
	case EHRBCardCombinationType::StraightFlush:
	{
		// Highest card's rank, SubRankValue = suit rank (all same suit)
		int32 MaxRank = -1;
		for (const FHRBCardData& Card : Cards)
		{
			if (Card.GetRank() > MaxRank)
			{
				MaxRank = Card.GetRank();
			}
		}
		Combination.RankValue = MaxRank;
		Combination.SubRankValue = Cards[0].GetSuitRank(); // all same suit
		break;
	}
	default:
		break;
	}

	return Combination;
}

TArray<FHRBPlayedCombination> UHRBLexioRuleEngine::FindAllValidCombinations(
	const TArray<FHRBCardData>& Hand,
	EHRBCardCombinationType RequiredType) const
{
	TArray<FHRBPlayedCombination> Results;

	const bool bWantFiveCard = (RequiredType == EHRBCardCombinationType::None) || IsFiveCardType(RequiredType);
	const bool bWantSmallCard = (RequiredType == EHRBCardCombinationType::None)
		|| RequiredType == EHRBCardCombinationType::Single
		|| RequiredType == EHRBCardCombinationType::Pair
		|| RequiredType == EHRBCardCombinationType::Triple;

	// ---- 1/2/3-card combinations ----
	if (bWantSmallCard)
	{
		// Group cards by number
		TMap<int32, TArray<FHRBCardData>> CardsByNumber;
		for (const FHRBCardData& Card : Hand)
		{
			CardsByNumber.FindOrAdd(Card.Number).Add(Card);
		}

		for (const auto& Pair : CardsByNumber)
		{
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
	}

	// ---- 5-card combinations ----
	if (bWantFiveCard && Hand.Num() >= 5)
	{
		// Group by number and by suit for efficient generation
		TMap<int32, TArray<FHRBCardData>> CardsByNumber;
		TMap<EHRBCardSuit, TArray<FHRBCardData>> CardsBySuit;
		// Also group by rank for straight detection
		TMap<int32, TArray<FHRBCardData>> CardsByRank;

		for (const FHRBCardData& Card : Hand)
		{
			CardsByNumber.FindOrAdd(Card.Number).Add(Card);
			CardsBySuit.FindOrAdd(Card.Suit).Add(Card);
			CardsByRank.FindOrAdd(Card.GetRank()).Add(Card);
		}

		// --- Straights and StraightFlushes ---
		// Valid straight starting ranks: the 5 consecutive ranks must all exist
		// Ranks go 0..8 (numbers 3,4,5,6,7,8,9,1,2)
		// Valid straights: 0-1-2-3-4, 1-2-3-4-5, 2-3-4-5-6, 3-4-5-6-7, 4-5-6-7-8
		// (i.e., starting rank 0 through 4)
		for (int32 StartRank = 0; StartRank <= 4; ++StartRank)
		{
			// Check if hand has at least one card at each of the 5 ranks
			bool bCanMakeStraight = true;
			for (int32 r = 0; r < 5; ++r)
			{
				if (!CardsByRank.Contains(StartRank + r) || CardsByRank[StartRank + r].Num() == 0)
				{
					bCanMakeStraight = false;
					break;
				}
			}

			if (!bCanMakeStraight)
			{
				continue;
			}

			// Generate all combinations: one card per rank
			// Use 5 nested indices into the cards at each rank
			const TArray<FHRBCardData>& R0 = CardsByRank[StartRank];
			const TArray<FHRBCardData>& R1 = CardsByRank[StartRank + 1];
			const TArray<FHRBCardData>& R2 = CardsByRank[StartRank + 2];
			const TArray<FHRBCardData>& R3 = CardsByRank[StartRank + 3];
			const TArray<FHRBCardData>& R4 = CardsByRank[StartRank + 4];

			for (int32 i0 = 0; i0 < R0.Num(); ++i0)
			{
				for (int32 i1 = 0; i1 < R1.Num(); ++i1)
				{
					for (int32 i2 = 0; i2 < R2.Num(); ++i2)
					{
						for (int32 i3 = 0; i3 < R3.Num(); ++i3)
						{
							for (int32 i4 = 0; i4 < R4.Num(); ++i4)
							{
								TArray<FHRBCardData> ComboCards;
								ComboCards.Add(R0[i0]);
								ComboCards.Add(R1[i1]);
								ComboCards.Add(R2[i2]);
								ComboCards.Add(R3[i3]);
								ComboCards.Add(R4[i4]);

								EHRBCardCombinationType DetectedType = DetermineCombinationType(ComboCards);
								// This will be either Straight or StraightFlush
								if (DetectedType == EHRBCardCombinationType::Straight || DetectedType == EHRBCardCombinationType::StraightFlush)
								{
									Results.Add(MakePlayedCombination(ComboCards));
								}
							}
						}
					}
				}
			}
		}

		// --- Flushes (non-straight) ---
		for (const auto& SuitPair : CardsBySuit)
		{
			const TArray<FHRBCardData>& SuitCards = SuitPair.Value;
			if (SuitCards.Num() < 5)
			{
				continue;
			}

			// Generate C(n, 5) combinations
			const int32 N = SuitCards.Num();
			for (int32 i0 = 0; i0 < N - 4; ++i0)
			{
				for (int32 i1 = i0 + 1; i1 < N - 3; ++i1)
				{
					for (int32 i2 = i1 + 1; i2 < N - 2; ++i2)
					{
						for (int32 i3 = i2 + 1; i3 < N - 1; ++i3)
						{
							for (int32 i4 = i3 + 1; i4 < N; ++i4)
							{
								TArray<FHRBCardData> ComboCards;
								ComboCards.Add(SuitCards[i0]);
								ComboCards.Add(SuitCards[i1]);
								ComboCards.Add(SuitCards[i2]);
								ComboCards.Add(SuitCards[i3]);
								ComboCards.Add(SuitCards[i4]);

								EHRBCardCombinationType DetectedType = DetermineCombinationType(ComboCards);
								// Only add Flush here (StraightFlush already generated above)
								if (DetectedType == EHRBCardCombinationType::Flush)
								{
									Results.Add(MakePlayedCombination(ComboCards));
								}
							}
						}
					}
				}
			}
		}

		// --- FullHouse ---
		// For each number with 3+ cards (triple), for each other number with 2+ cards (pair)
		for (const auto& TriplePair : CardsByNumber)
		{
			const int32 TripleNumber = TriplePair.Key;
			const TArray<FHRBCardData>& TripleCards = TriplePair.Value;
			if (TripleCards.Num() < 3)
			{
				continue;
			}

			// Generate all triples from this number
			for (int32 ti = 0; ti < TripleCards.Num() - 2; ++ti)
			{
				for (int32 tj = ti + 1; tj < TripleCards.Num() - 1; ++tj)
				{
					for (int32 tk = tj + 1; tk < TripleCards.Num(); ++tk)
					{
						// For each other number with 2+ cards
						for (const auto& PairPair : CardsByNumber)
						{
							const int32 PairNumber = PairPair.Key;
							if (PairNumber == TripleNumber)
							{
								continue;
							}
							const TArray<FHRBCardData>& PairCards = PairPair.Value;
							if (PairCards.Num() < 2)
							{
								continue;
							}

							// Generate all pairs from this number
							for (int32 pi = 0; pi < PairCards.Num() - 1; ++pi)
							{
								for (int32 pj = pi + 1; pj < PairCards.Num(); ++pj)
								{
									TArray<FHRBCardData> ComboCards;
									ComboCards.Add(TripleCards[ti]);
									ComboCards.Add(TripleCards[tj]);
									ComboCards.Add(TripleCards[tk]);
									ComboCards.Add(PairCards[pi]);
									ComboCards.Add(PairCards[pj]);
									Results.Add(MakePlayedCombination(ComboCards));
								}
							}
						}
					}
				}
			}
		}

		// --- FourOfAKind ---
		// For each number with 4 cards, for each remaining card
		for (const auto& QuadPair : CardsByNumber)
		{
			const int32 QuadNumber = QuadPair.Key;
			const TArray<FHRBCardData>& QuadCards = QuadPair.Value;
			if (QuadCards.Num() < 4)
			{
				continue;
			}

			// The four cards
			TArray<FHRBCardData> FourCards;
			FourCards.Add(QuadCards[0]);
			FourCards.Add(QuadCards[1]);
			FourCards.Add(QuadCards[2]);
			FourCards.Add(QuadCards[3]);

			// For each other card in hand as the kicker
			for (const FHRBCardData& Kicker : Hand)
			{
				if (Kicker.Number == QuadNumber)
				{
					continue;
				}

				TArray<FHRBCardData> ComboCards = FourCards;
				ComboCards.Add(Kicker);
				Results.Add(MakePlayedCombination(ComboCards));
			}
		}
	}

	// Sort results: by card count, then by type tier, then by RankValue, then SubRankValue
	Results.Sort([](const FHRBPlayedCombination& A, const FHRBPlayedCombination& B)
	{
		const int32 CountA = A.Cards.Num();
		const int32 CountB = B.Cards.Num();
		if (CountA != CountB)
		{
			return CountA < CountB;
		}

		// Within same card count, sort by type tier
		const uint8 TypeA = static_cast<uint8>(A.Type);
		const uint8 TypeB = static_cast<uint8>(B.Type);
		if (TypeA != TypeB)
		{
			return TypeA < TypeB;
		}

		if (A.RankValue != B.RankValue)
		{
			return A.RankValue < B.RankValue;
		}

		return A.SubRankValue < B.SubRankValue;
	});

	return Results;
}
