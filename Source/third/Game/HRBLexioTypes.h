// Copyright HRB Lexio Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HRBLexioTypes.generated.h"

/**
 * Card suit types in Lexio.
 */
UENUM(BlueprintType)
enum class EHRBCardSuit : uint8
{
	Cloud,  // 구름 (weakest)
	Star,   // 별
	Moon,   // 달
	Sun     // 해 (strongest)
};

/**
 * Card combination types supported in Lexio.
 */
UENUM(BlueprintType)
enum class EHRBCardCombinationType : uint8
{
	None,
	Single,
	Pair,
	Triple,
	Straight,       // 5 consecutive numbers (mixed suits)
	Flush,          // 5 same suit (any numbers)
	FullHouse,      // 3 of a kind + 2 of a kind
	FourOfAKind,    // 4 of a kind + 1 any
	StraightFlush   // 5 consecutive same suit
};

/**
 * Represents a single card in the deck.
 */
USTRUCT(BlueprintType)
struct FHRBCardData
{
	GENERATED_BODY()

	/** Card number (1~9) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Number = 0;

	/** Card suit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHRBCardSuit Suit = EHRBCardSuit::Cloud;

	/** Unique instance ID (0~35) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InstanceId = 0;

	bool operator==(const FHRBCardData& Other) const;
	bool operator<(const FHRBCardData& Other) const;

	/**
	 * Returns the rank of this card for comparison purposes.
	 * Lexio ranking: 3(weakest) < 4 < 5 < 6 < 7 < 8 < 9 < 1 < 2(strongest)
	 * Rank 0 = Number 3, Rank 1 = Number 4, ... Rank 6 = Number 9, Rank 7 = Number 1, Rank 8 = Number 2
	 */
	int32 GetRank() const;

	/** Returns the suit rank for comparison. Cloud=0, Star=1, Moon=2, Sun=3 */
	int32 GetSuitRank() const;

	FString ToString() const;
};

/**
 * Represents a played combination (submitted to the table).
 */
USTRUCT(BlueprintType)
struct FHRBPlayedCombination
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHRBCardCombinationType Type = EHRBCardCombinationType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FHRBCardData> Cards;

	/** The rank value used for comparison (highest card rank in the combination, based on Lexio ranking) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RankValue = 0;

	/** Sub-rank value used for suit-based tiebreaking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SubRankValue = 0;

	/** Returns true if this combination has a valid type and cards. */
	bool IsValid() const;

	/** Returns true if this combination can beat the Other combination. */
	bool CanBeatOther(const FHRBPlayedCombination& Other) const;

	/** Returns the tier of a 5-card combination for cross-type comparison. */
	int32 GetFiveCardTier() const;

	FString ToString() const;
};
