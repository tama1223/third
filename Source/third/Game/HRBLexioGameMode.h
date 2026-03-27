// Copyright HRB Lexio Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HRBLexioGameMode.generated.h"

class UHRBLexioGameState;

/**
 * Test GameMode that auto-simulates one full Lexio game in BeginPlay.
 * Outputs the entire game flow via UE_LOG.
 */
UCLASS()
class AHRBLexioGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHRBLexioGameMode();

protected:
	virtual void BeginPlay() override;

private:
	/** Run one full auto-simulated game. */
	void RunSimulation();

	/** Log a player's hand. */
	void LogPlayerHand(int32 PlayerIndex, const TArray<struct FHRBCardData>& Hand) const;

	UPROPERTY()
	TObjectPtr<UHRBLexioGameState> LexioGameState;
};
