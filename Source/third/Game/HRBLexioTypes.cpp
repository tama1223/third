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
	return Number < Other.Number;
}

FString FHRBCardData::ToString() const
{
	return FString::Printf(TEXT("%d"), Number);
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
	default:
		return false;
	}
}

bool FHRBPlayedCombination::CanBeatOther(const FHRBPlayedCombination& Other) const
{
	// Must be the same combination type
	if (Type != Other.Type)
	{
		return false;
	}

	// Must have a higher rank value
	return RankValue > Other.RankValue;
}

FString FHRBPlayedCombination::ToString() const
{
	FString TypeStr;
	switch (Type)
	{
	case EHRBCardCombinationType::Single: TypeStr = TEXT("Single"); break;
	case EHRBCardCombinationType::Pair: TypeStr = TEXT("Pair"); break;
	case EHRBCardCombinationType::Triple: TypeStr = TEXT("Triple"); break;
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

	return FString::Printf(TEXT("%s %s"), *TypeStr, *CardsStr);
}
