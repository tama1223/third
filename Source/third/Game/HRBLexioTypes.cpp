// Copyright HRB Lexio Project. All Rights Reserved.

#include "HRBLexioTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HRBLexioTypes)

// ----- FHRBCardData -----

bool FHRBCardData::operator==(const FHRBCardData& Other) const
{
	return InstanceId == Other.InstanceId;
}

bool FHRBCardData::operator<(const FHRBCardData& Other) const
{
	if (GetRank() != Other.GetRank())
	{
		return GetRank() < Other.GetRank();
	}
	return GetSuitRank() < Other.GetSuitRank();
}

int32 FHRBCardData::GetRank() const
{
	// Lexio ranking: 3(weakest=0) < 4(1) < 5(2) < 6(3) < 7(4) < 8(5) < 9(6) < 1(7) < 2(strongest=8)
	if (Number >= 3 && Number <= 9)
	{
		return Number - 3; // 3->0, 4->1, ..., 9->6
	}
	if (Number == 1)
	{
		return 7;
	}
	if (Number == 2)
	{
		return 8;
	}
	// Fallback for invalid numbers
	return -1;
}

int32 FHRBCardData::GetSuitRank() const
{
	return static_cast<int32>(Suit);
}

FString FHRBCardData::ToString() const
{
	const TCHAR* SuitNames[] = { TEXT("Cloud"), TEXT("Star"), TEXT("Moon"), TEXT("Sun") };
	int32 SuitIdx = static_cast<int32>(Suit);
	if (SuitIdx >= 0 && SuitIdx <= 3)
	{
		return FString::Printf(TEXT("%s%d"), SuitNames[SuitIdx], Number);
	}
	return FString::Printf(TEXT("?%d"), Number);
}

// ----- FHRBPlayedCombination -----

bool FHRBPlayedCombination::IsValid() const
{
	if (Type == EHRBCardCombinationType::None || Cards.Num() == 0)
	{
		return false;
	}

	switch (Type)
	{
	case EHRBCardCombinationType::Single:
		return Cards.Num() == 1;
	case EHRBCardCombinationType::Pair:
		return Cards.Num() == 2 && Cards[0].Number == Cards[1].Number;
	case EHRBCardCombinationType::Triple:
		return Cards.Num() == 3 && Cards[0].Number == Cards[1].Number && Cards[1].Number == Cards[2].Number;
	case EHRBCardCombinationType::Straight:
	case EHRBCardCombinationType::Flush:
	case EHRBCardCombinationType::FullHouse:
	case EHRBCardCombinationType::FourOfAKind:
	case EHRBCardCombinationType::StraightFlush:
		return Cards.Num() == 5;
	default:
		return false;
	}
}

int32 FHRBPlayedCombination::GetFiveCardTier() const
{
	switch (Type)
	{
	case EHRBCardCombinationType::Straight:       return 0;
	case EHRBCardCombinationType::Flush:          return 1;
	case EHRBCardCombinationType::FullHouse:      return 2;
	case EHRBCardCombinationType::FourOfAKind:    return 3;
	case EHRBCardCombinationType::StraightFlush:  return 4;
	default: return -1;
	}
}

bool FHRBPlayedCombination::CanBeatOther(const FHRBPlayedCombination& Other) const
{
	const int32 MyTier = GetFiveCardTier();
	const int32 OtherTier = Other.GetFiveCardTier();
	const bool bMyIsFiveCard = (MyTier >= 0);
	const bool bOtherIsFiveCard = (OtherTier >= 0);

	// 5-card vs 5-card
	if (bMyIsFiveCard && bOtherIsFiveCard)
	{
		if (MyTier != OtherTier)
		{
			return MyTier > OtherTier;
		}
		// Same 5-card type: compare RankValue then SubRankValue
		if (RankValue != Other.RankValue)
		{
			return RankValue > Other.RankValue;
		}
		return SubRankValue > Other.SubRankValue;
	}

	// 5-card vs non-5-card (or vice versa): cannot beat
	if (bMyIsFiveCard != bOtherIsFiveCard)
	{
		return false;
	}

	// 1/2/3-card combos: must be same type
	if (Type != Other.Type)
	{
		return false;
	}

	// Same type: compare RankValue first, then SubRankValue
	if (RankValue != Other.RankValue)
	{
		return RankValue > Other.RankValue;
	}
	return SubRankValue > Other.SubRankValue;
}

FString FHRBPlayedCombination::ToString() const
{
	FString TypeStr;
	switch (Type)
	{
	case EHRBCardCombinationType::Single:        TypeStr = TEXT("Single"); break;
	case EHRBCardCombinationType::Pair:          TypeStr = TEXT("Pair"); break;
	case EHRBCardCombinationType::Triple:        TypeStr = TEXT("Triple"); break;
	case EHRBCardCombinationType::Straight:      TypeStr = TEXT("Straight"); break;
	case EHRBCardCombinationType::Flush:         TypeStr = TEXT("Flush"); break;
	case EHRBCardCombinationType::FullHouse:     TypeStr = TEXT("FullHouse"); break;
	case EHRBCardCombinationType::FourOfAKind:   TypeStr = TEXT("FourOfAKind"); break;
	case EHRBCardCombinationType::StraightFlush: TypeStr = TEXT("StraightFlush"); break;
	default: TypeStr = TEXT("None"); break;
	}

	FString CardsStr = TEXT("[");
	for (int32 i = 0; i < Cards.Num(); ++i)
	{
		if (i > 0)
		{
			CardsStr += TEXT(",");
		}
		CardsStr += Cards[i].ToString();
	}
	CardsStr += TEXT("]");

	return FString::Printf(TEXT("%s %s (R:%d S:%d)"), *TypeStr, *CardsStr, RankValue, SubRankValue);
}
