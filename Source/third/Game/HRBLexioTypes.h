// Copyright HRB Lexio Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HRBLexioTypes.generated.h"

/**
 * Card combination types supported in Lexio MVP.
 */
UENUM(BlueprintType)
enum class EHRBCardCombinationType : uint8
{
	None,
	Single,
	Pair,
	Triple
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

	/** Unique instance ID (0~35) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InstanceId = 0;

	bool operator==(const FHRBCardData& Other) const;
	bool operator<(const FHRBCardData& Other) const;

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

	/** The rank value used for comparison (highest card number in the combination) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RankValue = 0;

	/** Returns true if this combination has a valid type and cards. */
	bool IsValid() const;

	/** Returns true if this combination can beat the Other combination. */
	bool CanBeatOther(const FHRBPlayedCombination& Other) const;

	FString ToString() const;
};
