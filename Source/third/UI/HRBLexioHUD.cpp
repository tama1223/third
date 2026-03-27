// Copyright HRB Lexio Project. All Rights Reserved.

#include "HRBLexioHUD.h"
#include "Game/HRBLexioGameState.h"
#include "Game/HRBLexioGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HRBLexioHUD)

AHRBLexioHUD::AHRBLexioHUD()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AHRBLexioHUD::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine)
	{
		HUDFont = GEngine->GetLargeFont();
	}
}

void AHRBLexioHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (StatusMessageTimer > 0.0f)
	{
		StatusMessageTimer -= DeltaSeconds;
		if (StatusMessageTimer <= 0.0f)
		{
			CurrentStatusMessage.Empty();
			StatusMessageTimer = 0.0f;
		}
	}
}

void AHRBLexioHUD::SetGameState(UHRBLexioGameState* InGameState)
{
	GameState = InGameState;
}

void AHRBLexioHUD::ShowStatusMessage(const FString& Message, float Duration)
{
	CurrentStatusMessage = Message;
	StatusMessageTimer = Duration;
}

void AHRBLexioHUD::DrawHUD()
{
	Super::DrawHUD();

	// Cache viewport size for use outside DrawHUD (e.g., HandleClick)
	if (Canvas)
	{
		CachedViewportSize = FVector2D(Canvas->SizeX, Canvas->SizeY);
	}

	if (!GameState)
	{
		return;
	}

	DrawAIInfo();
	DrawTurnInfo();
	DrawTableCombination();
	DrawStatusMessage();
	DrawPlayerHand();
	DrawButtons();
	DrawGameOverMessage();
}

void AHRBLexioHUD::DrawPlayerHand()
{
	const TArray<FHRBCardData>& Hand = GameState->GetPlayerHand(HumanPlayerIndex);
	const int32 NumCards = Hand.Num();
	if (NumCards == 0)
	{
		return;
	}

	const FVector2D CardSize = GetCardSize();

	for (int32 i = 0; i < NumCards; ++i)
	{
		const FVector2D Pos = GetCardPosition(i, NumCards);
		const bool bSelected = SelectedCardIndices.Contains(i);
		const float OffsetY = bSelected ? SelectedOffsetY : 0.0f;
		const FVector2D DrawPos(Pos.X, Pos.Y + OffsetY);

		// Card border
		const FLinearColor BorderColor = bSelected ? FLinearColor::Yellow : FLinearColor::Black;
		const float BorderThickness = bSelected ? 3.0f : 2.0f;

		// Draw card background (white)
		DrawRect(FLinearColor::White, DrawPos.X, DrawPos.Y, CardSize.X, CardSize.Y);

		// Draw border (4 thin rects)
		DrawRect(BorderColor, DrawPos.X, DrawPos.Y, CardSize.X, BorderThickness); // Top
		DrawRect(BorderColor, DrawPos.X, DrawPos.Y + CardSize.Y - BorderThickness, CardSize.X, BorderThickness); // Bottom
		DrawRect(BorderColor, DrawPos.X, DrawPos.Y, BorderThickness, CardSize.Y); // Left
		DrawRect(BorderColor, DrawPos.X + CardSize.X - BorderThickness, DrawPos.Y, BorderThickness, CardSize.Y); // Right

		// Draw card number centered
		const FString NumberStr = FString::Printf(TEXT("%d"), Hand[i].Number);
		float TextW, TextH;
		GetTextSize(NumberStr, TextW, TextH, HUDFont, 1.0f);

		const float TextX = DrawPos.X + (CardSize.X - TextW) * 0.5f;
		const float TextY = DrawPos.Y + (CardSize.Y - TextH) * 0.5f;
		DrawText(NumberStr, FLinearColor::Black, TextX, TextY, HUDFont, 1.0f);
	}
}

void AHRBLexioHUD::DrawTableCombination()
{
	const FHRBPlayedCombination& TableCombo = GameState->GetCurrentTableCombination();

	const float CenterX = CachedViewportSize.X * 0.5f;
	const float CenterY = CachedViewportSize.Y * 0.4f;

	if (!TableCombo.IsValid())
	{
		// Draw "New Round" text
		const FString Text = TEXT("New Round - Play any combination");
		float TextW, TextH;
		GetTextSize(Text, TextW, TextH, HUDFont, 1.0f);
		DrawText(Text, FLinearColor(0.5f, 0.5f, 0.5f), CenterX - TextW * 0.5f, CenterY, HUDFont, 1.0f);
		return;
	}

	// Draw table cards
	const int32 NumCards = TableCombo.Cards.Num();
	const float TotalWidth = NumCards * TableCardWidth + (NumCards - 1) * CardSpacing;
	const float StartX = CenterX - TotalWidth * 0.5f;

	for (int32 i = 0; i < NumCards; ++i)
	{
		const float CardX = StartX + i * (TableCardWidth + CardSpacing);
		const float CardY = CenterY - TableCardHeight * 0.5f;

		// Card background (light blue)
		DrawRect(FLinearColor(0.85f, 0.92f, 1.0f), CardX, CardY, TableCardWidth, TableCardHeight);

		// Border
		const float BT = 2.0f;
		DrawRect(FLinearColor(0.2f, 0.4f, 0.7f), CardX, CardY, TableCardWidth, BT);
		DrawRect(FLinearColor(0.2f, 0.4f, 0.7f), CardX, CardY + TableCardHeight - BT, TableCardWidth, BT);
		DrawRect(FLinearColor(0.2f, 0.4f, 0.7f), CardX, CardY, BT, TableCardHeight);
		DrawRect(FLinearColor(0.2f, 0.4f, 0.7f), CardX + TableCardWidth - BT, CardY, BT, TableCardHeight);

		// Number
		const FString NumStr = FString::Printf(TEXT("%d"), TableCombo.Cards[i].Number);
		float TextW, TextH;
		GetTextSize(NumStr, TextW, TextH, HUDFont, 1.2f);
		DrawText(NumStr, FLinearColor(0.1f, 0.1f, 0.3f),
			CardX + (TableCardWidth - TextW) * 0.5f,
			CardY + (TableCardHeight - TextH) * 0.5f,
			HUDFont, 1.2f);
	}

	// Draw combination type label below table cards
	FString TypeLabel;
	switch (TableCombo.Type)
	{
	case EHRBCardCombinationType::Single: TypeLabel = TEXT("Single"); break;
	case EHRBCardCombinationType::Pair: TypeLabel = TEXT("Pair"); break;
	case EHRBCardCombinationType::Triple: TypeLabel = TEXT("Triple"); break;
	default: TypeLabel = TEXT(""); break;
	}

	if (!TypeLabel.IsEmpty())
	{
		float TextW, TextH;
		GetTextSize(TypeLabel, TextW, TextH, HUDFont, 1.0f);
		DrawText(TypeLabel, FLinearColor(0.3f, 0.3f, 0.3f),
			CenterX - TextW * 0.5f,
			CenterY + TableCardHeight * 0.5f + 10.0f,
			HUDFont, 1.0f);
	}
}

void AHRBLexioHUD::DrawAIInfo()
{
	const float Margin = 30.0f;
	const float TopY = 30.0f;

	// AI Player 1 (left side)
	{
		const int32 HandCount = GameState->GetPlayerHand(1).Num();
		const FString Text = FString::Printf(TEXT("AI 1: %d cards"), HandCount);
		DrawText(Text, FLinearColor::White, Margin, TopY, HUDFont, 1.0f);
	}

	// AI Player 2 (right side)
	{
		const int32 HandCount = GameState->GetPlayerHand(2).Num();
		const FString Text = FString::Printf(TEXT("AI 2: %d cards"), HandCount);
		float TextW, TextH;
		GetTextSize(Text, TextW, TextH, HUDFont, 1.0f);
		DrawText(Text, FLinearColor::White, CachedViewportSize.X - Margin - TextW, TopY, HUDFont, 1.0f);
	}
}

void AHRBLexioHUD::DrawTurnInfo()
{
	const float CenterX = CachedViewportSize.X * 0.5f;
	const float TopY = 60.0f;

	// Current turn
	const int32 CurrentPlayer = GameState->GetCurrentPlayerIndex();
	FString PlayerName;
	if (CurrentPlayer == HumanPlayerIndex)
	{
		PlayerName = TEXT("Your Turn");
	}
	else
	{
		PlayerName = FString::Printf(TEXT("AI %d's Turn"), CurrentPlayer);
	}

	float TextW, TextH;
	GetTextSize(PlayerName, TextW, TextH, HUDFont, 1.2f);

	const FLinearColor TurnColor = (CurrentPlayer == HumanPlayerIndex)
		? FLinearColor::Green
		: FLinearColor(1.0f, 0.7f, 0.3f);

	DrawText(PlayerName, TurnColor, CenterX - TextW * 0.5f, TopY, HUDFont, 1.2f);
}

void AHRBLexioHUD::DrawButtons()
{
	// Only show buttons on human player's turn
	if (GameState->IsGameOver() || GameState->GetCurrentPlayerIndex() != HumanPlayerIndex)
	{
		return;
	}

	const float CenterX = CachedViewportSize.X * 0.5f;
	const float BottomY = CachedViewportSize.Y - 50.0f;

	// Submit button
	SubmitButtonSize = FVector2D(ButtonWidth, ButtonHeight);
	SubmitButtonPos = FVector2D(CenterX - ButtonWidth - ButtonSpacing * 0.5f, BottomY - ButtonHeight);

	const FLinearColor SubmitBgColor(0.2f, 0.6f, 0.3f);
	DrawRect(SubmitBgColor, SubmitButtonPos.X, SubmitButtonPos.Y, SubmitButtonSize.X, SubmitButtonSize.Y);

	// Submit text
	{
		const FString Text = TEXT("Submit");
		float TextW, TextH;
		GetTextSize(Text, TextW, TextH, HUDFont, 1.0f);
		DrawText(Text, FLinearColor::White,
			SubmitButtonPos.X + (SubmitButtonSize.X - TextW) * 0.5f,
			SubmitButtonPos.Y + (SubmitButtonSize.Y - TextH) * 0.5f,
			HUDFont, 1.0f);
	}

	// Pass button
	PassButtonSize = FVector2D(ButtonWidth, ButtonHeight);
	PassButtonPos = FVector2D(CenterX + ButtonSpacing * 0.5f, BottomY - ButtonHeight);

	// Cannot pass on empty table
	const bool bCanPass = GameState->GetCurrentTableCombination().IsValid();
	const FLinearColor PassBgColor = bCanPass
		? FLinearColor(0.6f, 0.3f, 0.2f)
		: FLinearColor(0.4f, 0.4f, 0.4f);

	DrawRect(PassBgColor, PassButtonPos.X, PassButtonPos.Y, PassButtonSize.X, PassButtonSize.Y);

	// Pass text
	{
		const FString Text = TEXT("Pass");
		float TextW, TextH;
		GetTextSize(Text, TextW, TextH, HUDFont, 1.0f);
		DrawText(Text, FLinearColor::White,
			PassButtonPos.X + (PassButtonSize.X - TextW) * 0.5f,
			PassButtonPos.Y + (PassButtonSize.Y - TextH) * 0.5f,
			HUDFont, 1.0f);
	}
}

void AHRBLexioHUD::DrawStatusMessage()
{
	if (CurrentStatusMessage.IsEmpty())
	{
		return;
	}

	const float CenterX = CachedViewportSize.X * 0.5f;
	const float Y = CachedViewportSize.Y * 0.55f;

	float TextW, TextH;
	GetTextSize(CurrentStatusMessage, TextW, TextH, HUDFont, 1.0f);

	// Background for visibility
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f),
		CenterX - TextW * 0.5f - 10.0f, Y - 5.0f,
		TextW + 20.0f, TextH + 10.0f);

	DrawText(CurrentStatusMessage, FLinearColor(1.0f, 0.3f, 0.3f),
		CenterX - TextW * 0.5f, Y, HUDFont, 1.0f);
}

void AHRBLexioHUD::DrawGameOverMessage()
{
	if (!GameState->IsGameOver())
	{
		return;
	}

	const float CenterX = CachedViewportSize.X * 0.5f;
	const float CenterY = CachedViewportSize.Y * 0.5f;

	const int32 Winner = GameState->GetWinnerIndex();
	FString WinnerText;
	if (Winner == HumanPlayerIndex)
	{
		WinnerText = TEXT("You Win!");
	}
	else
	{
		WinnerText = FString::Printf(TEXT("AI %d Wins!"), Winner);
	}

	float TextW, TextH;
	GetTextSize(WinnerText, TextW, TextH, HUDFont, 2.0f);

	// Dark overlay background
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f),
		CenterX - TextW * 0.5f - 30.0f, CenterY - TextH * 0.5f - 20.0f,
		TextW + 60.0f, TextH + 40.0f);

	const FLinearColor WinColor = (Winner == HumanPlayerIndex)
		? FLinearColor(1.0f, 0.85f, 0.0f)
		: FLinearColor(1.0f, 0.4f, 0.4f);

	DrawText(WinnerText, WinColor, CenterX - TextW * 0.5f, CenterY - TextH * 0.5f, HUDFont, 2.0f);
}

FVector2D AHRBLexioHUD::GetCardPosition(int32 Index, int32 TotalCards) const
{
	// Canvas is only valid during DrawHUD(). Use cached viewport size otherwise.
	float ViewportW = CachedViewportSize.X;
	float ViewportH = CachedViewportSize.Y;

	const float TotalWidth = TotalCards * CardWidth + (TotalCards - 1) * CardSpacing;
	const float StartX = (ViewportW - TotalWidth) * 0.5f;
	const float Y = ViewportH - CardHeight - 100.0f;

	return FVector2D(StartX + Index * (CardWidth + CardSpacing), Y);
}

FVector2D AHRBLexioHUD::GetCardSize() const
{
	return FVector2D(CardWidth, CardHeight);
}

bool AHRBLexioHUD::IsPointInRect(const FVector2D& Point, const FVector2D& RectPos, const FVector2D& RectSize) const
{
	return Point.X >= RectPos.X && Point.X <= RectPos.X + RectSize.X
		&& Point.Y >= RectPos.Y && Point.Y <= RectPos.Y + RectSize.Y;
}

void AHRBLexioHUD::HandleClick(const FVector2D& ClickPosition)
{
	if (!GameState || GameState->IsGameOver())
	{
		return;
	}

	// Only respond to clicks on human player's turn
	if (GameState->GetCurrentPlayerIndex() != HumanPlayerIndex)
	{
		return;
	}

	const FVector2D CardSize = GetCardSize();

	// Check card clicks
	const TArray<FHRBCardData>& Hand = GameState->GetPlayerHand(HumanPlayerIndex);
	const int32 NumCards = Hand.Num();

	for (int32 i = NumCards - 1; i >= 0; --i) // Reverse order for overlap priority
	{
		FVector2D Pos = GetCardPosition(i, NumCards);
		const bool bSelected = SelectedCardIndices.Contains(i);
		if (bSelected)
		{
			Pos.Y += SelectedOffsetY;
		}

		if (IsPointInRect(ClickPosition, Pos, CardSize))
		{
			// Toggle selection
			if (bSelected)
			{
				SelectedCardIndices.Remove(i);
			}
			else
			{
				SelectedCardIndices.Add(i);
			}
			return;
		}
	}

	// Check Submit button
	if (IsPointInRect(ClickPosition, SubmitButtonPos, SubmitButtonSize))
	{
		if (SelectedCardIndices.Num() == 0)
		{
			ShowStatusMessage(TEXT("Select cards first!"));
			return;
		}

		// Gather selected cards
		TArray<FHRBCardData> SelectedCards;
		for (int32 Idx : SelectedCardIndices)
		{
			if (Idx < Hand.Num())
			{
				SelectedCards.Add(Hand[Idx]);
			}
		}

		// Try to submit via GameMode
		AHRBLexioGameMode* GM = Cast<AHRBLexioGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			bool bSuccess = GM->ProcessHumanTurn(SelectedCards);
			if (bSuccess)
			{
				SelectedCardIndices.Empty();
			}
			else
			{
				ShowStatusMessage(TEXT("Invalid combination!"));
			}
		}
		return;
	}

	// Check Pass button
	if (IsPointInRect(ClickPosition, PassButtonPos, PassButtonSize))
	{
		if (!GameState->GetCurrentTableCombination().IsValid())
		{
			ShowStatusMessage(TEXT("Cannot pass on empty table!"));
			return;
		}

		AHRBLexioGameMode* GM = Cast<AHRBLexioGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			GM->ProcessHumanPass();
			SelectedCardIndices.Empty();
		}
		return;
	}
}
