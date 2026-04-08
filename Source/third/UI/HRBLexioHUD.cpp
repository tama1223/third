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

	// 배경 (어두운 녹색 테이블)
	DrawRect(FLinearColor(0.05f, 0.15f, 0.08f), 0.0f, 0.0f, CachedViewportSize.X, CachedViewportSize.Y);

	DrawAIInfo();
	DrawRoundInfo();
	DrawScoreInfo();
	DrawTurnInfo();
	DrawTableCombination();
	DrawRankLegend();
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

		// Suit color
		const FLinearColor SuitCol = GetSuitColor(Hand[i].Suit);
		const FLinearColor DarkSuitCol(SuitCol.R * 0.6f, SuitCol.G * 0.6f, SuitCol.B * 0.6f);

		// Card border
		const FLinearColor BorderColor = bSelected ? FLinearColor::Yellow : DarkSuitCol;
		const float BorderThickness = bSelected ? 3.0f : 2.0f;

		// Draw card background (white)
		DrawRect(FLinearColor::White, DrawPos.X, DrawPos.Y, CardSize.X, CardSize.Y);

		// Draw border (4 thin rects)
		DrawRect(BorderColor, DrawPos.X, DrawPos.Y, CardSize.X, BorderThickness); // Top
		DrawRect(BorderColor, DrawPos.X, DrawPos.Y + CardSize.Y - BorderThickness, CardSize.X, BorderThickness); // Bottom
		DrawRect(BorderColor, DrawPos.X, DrawPos.Y, BorderThickness, CardSize.Y); // Left
		DrawRect(BorderColor, DrawPos.X + CardSize.X - BorderThickness, DrawPos.Y, BorderThickness, CardSize.Y); // Right

		// Draw card number in suit color
		const FString NumberStr = FString::Printf(TEXT("%d"), Hand[i].Number);
		float TextW, TextH;
		GetTextSize(NumberStr, TextW, TextH, HUDFont, 1.0f);

		const float TextX = DrawPos.X + (CardSize.X - TextW) * 0.5f;
		const float TextY = DrawPos.Y + (CardSize.Y - TextH) * 0.5f - 6.0f;
		DrawText(NumberStr, SuitCol, TextX, TextY, HUDFont, 1.0f);

		// Draw suit symbol below the number
		const FString SuitSym = GetSuitSymbol(Hand[i].Suit);
		float SuitW, SuitH;
		GetTextSize(SuitSym, SuitW, SuitH, HUDFont, 0.7f);
		const float SuitX = DrawPos.X + (CardSize.X - SuitW) * 0.5f;
		const float SuitY = TextY + TextH + 1.0f;
		DrawText(SuitSym, SuitCol, SuitX, SuitY, HUDFont, 0.7f);
	}
}

void AHRBLexioHUD::DrawTableCombination()
{
	const float CenterX = CachedViewportSize.X * 0.5f;
	const float CenterY = CachedViewportSize.Y * 0.4f;

	const auto& History = GameState->GetRoundHistory();
	const FHRBPlayedCombination& TableCombo = GameState->GetCurrentTableCombination();

	if (!TableCombo.IsValid() && History.Num() == 0)
	{
		const FString Text = TEXT("New Round - Play any combination");
		float TextW, TextH;
		GetTextSize(Text, TextW, TextH, HUDFont, 1.0f);
		DrawText(Text, FLinearColor(0.5f, 0.5f, 0.5f), CenterX - TextW * 0.5f, CenterY, HUDFont, 1.0f);
		return;
	}

	// 히스토리 전체를 가로로 나열 (이전 → 현재)
	const float HistoryCardW = 65.0f;
	const float HistoryCardH = 85.0f;
	const float GroupGap = 40.0f;  // 제출 그룹 간 간격
	const float InnerGap = 6.0f;   // 같은 제출 내 카드 간격
	const float BT = 2.0f;

	// 전체 폭 계산
	float TotalWidth = 0.0f;
	for (int32 g = 0; g < History.Num(); ++g)
	{
		const int32 NumCards = History[g].Combination.Cards.Num();
		TotalWidth += NumCards * HistoryCardW + (NumCards - 1) * InnerGap;
		if (g < History.Num() - 1)
		{
			TotalWidth += GroupGap;
		}
	}

	float DrawX = CenterX - TotalWidth * 0.5f;
	const float DrawY = CenterY - HistoryCardH * 0.5f;

	for (int32 g = 0; g < History.Num(); ++g)
	{
		const auto& Entry = History[g];
		const bool bIsLatest = (g == History.Num() - 1);

		// 플레이어 색상
		FLinearColor PColor;
		FString PName;
		if (Entry.PlayerIndex == HumanPlayerIndex) { PColor = FLinearColor(0.2f, 0.8f, 0.3f); PName = TEXT("You"); }
		else if (Entry.PlayerIndex == 1) { PColor = FLinearColor(1.0f, 0.4f, 0.4f); PName = TEXT("AI1"); }
		else { PColor = FLinearColor(0.4f, 0.6f, 1.0f); PName = TEXT("AI2"); }

		// 플레이어 이름 위에 표시
		float NameW, NameH;
		GetTextSize(PName, NameW, NameH, HUDFont, 0.9f);
		DrawText(PName, PColor, DrawX, DrawY - 25.0f, HUDFont, 0.9f);

		for (int32 c = 0; c < Entry.Combination.Cards.Num(); ++c)
		{
			const float CardX = DrawX + c * (HistoryCardW + InnerGap);

			// 이전 제출은 반투명, 최신은 불투명
			const float Alpha = bIsLatest ? 1.0f : 0.5f;
			const FLinearColor CardBg = bIsLatest
				? FLinearColor(0.85f, 0.92f, 1.0f, Alpha)
				: FLinearColor(0.7f, 0.75f, 0.8f, Alpha);
			const FLinearColor BorderCol = bIsLatest
				? FLinearColor(0.2f, 0.4f, 0.7f, Alpha)
				: FLinearColor(0.4f, 0.4f, 0.5f, Alpha);

			DrawRect(CardBg, CardX, DrawY, HistoryCardW, HistoryCardH);
			DrawRect(BorderCol, CardX, DrawY, HistoryCardW, BT);
			DrawRect(BorderCol, CardX, DrawY + HistoryCardH - BT, HistoryCardW, BT);
			DrawRect(BorderCol, CardX, DrawY, BT, HistoryCardH);
			DrawRect(BorderCol, CardX + HistoryCardW - BT, DrawY, BT, HistoryCardH);

			const FHRBCardData& CardData = Entry.Combination.Cards[c];
			const FLinearColor CardSuitCol = GetSuitColor(CardData.Suit);

			// Draw card number in suit color
			const FString NumStr = FString::Printf(TEXT("%d"), CardData.Number);
			float TextW, TextH;
			GetTextSize(NumStr, TextW, TextH, HUDFont, 1.5f);
			DrawText(NumStr, CardSuitCol,
				CardX + (HistoryCardW - TextW) * 0.5f,
				DrawY + (HistoryCardH - TextH) * 0.5f - 8.0f,
				HUDFont, 1.5f);

			// Draw suit symbol below the number
			const FString CardSuitSym = GetSuitSymbol(CardData.Suit);
			float SuitSymW, SuitSymH;
			GetTextSize(CardSuitSym, SuitSymW, SuitSymH, HUDFont, 0.7f);
			DrawText(CardSuitSym, CardSuitCol,
				CardX + (HistoryCardW - SuitSymW) * 0.5f,
				DrawY + (HistoryCardH - TextH) * 0.5f - 8.0f + TextH + 1.0f,
				HUDFont, 0.7f);
		}

		DrawX += Entry.Combination.Cards.Num() * (HistoryCardW + InnerGap) - InnerGap;

		// 그룹 사이에 화살표
		if (g < History.Num() - 1)
		{
			DrawText(TEXT(">"), FLinearColor(0.6f, 0.6f, 0.4f),
				DrawX + GroupGap * 0.25f, DrawY + HistoryCardH * 0.5f - 12.0f, HUDFont, 1.2f);
			DrawX += GroupGap;
		}
	}

	// Draw who played + combination type label below table cards
	const int32 LastPlayer = GameState->GetLastSubmitPlayerIndex();
	FString PlayerLabel;
	FLinearColor PlayerColor;
	if (LastPlayer == HumanPlayerIndex)
	{
		PlayerLabel = TEXT("You");
		PlayerColor = FLinearColor(0.2f, 0.8f, 0.3f); // 초록
	}
	else if (LastPlayer == 1)
	{
		PlayerLabel = TEXT("AI 1");
		PlayerColor = FLinearColor(1.0f, 0.4f, 0.4f); // 빨강
	}
	else if (LastPlayer == 2)
	{
		PlayerLabel = TEXT("AI 2");
		PlayerColor = FLinearColor(0.4f, 0.6f, 1.0f); // 파랑
	}

	FString TypeLabel;
	switch (TableCombo.Type)
	{
	case EHRBCardCombinationType::Single: TypeLabel = TEXT("Single"); break;
	case EHRBCardCombinationType::Pair: TypeLabel = TEXT("Pair"); break;
	case EHRBCardCombinationType::Triple: TypeLabel = TEXT("Triple"); break;
	case EHRBCardCombinationType::Straight: TypeLabel = TEXT("Straight"); break;
	case EHRBCardCombinationType::Flush: TypeLabel = TEXT("Flush"); break;
	case EHRBCardCombinationType::FullHouse: TypeLabel = TEXT("Full House"); break;
	case EHRBCardCombinationType::FourOfAKind: TypeLabel = TEXT("Four of a Kind"); break;
	case EHRBCardCombinationType::StraightFlush: TypeLabel = TEXT("Straight Flush"); break;
	default: TypeLabel = TEXT(""); break;
	}

	// "AI 1 - Pair" 형태로 표시
	FString FullLabel = FString::Printf(TEXT("%s - %s"), *PlayerLabel, *TypeLabel);
	{
		float TextW, TextH;
		GetTextSize(FullLabel, TextW, TextH, HUDFont, 1.0f);
		DrawText(FullLabel, PlayerColor,
			CenterX - TextW * 0.5f,
			CenterY + TableCardHeight * 0.5f + 10.0f,
			HUDFont, 1.0f);
	}
}

void AHRBLexioHUD::DrawAIInfo()
{
	const float Margin = 30.0f;
	const float TopY = 50.0f;
	const float AICardW = 35.0f;
	const float AICardH = 48.0f;
	const float AICardGap = 4.0f;
	const FLinearColor CardBackColor(0.3f, 0.3f, 0.6f);  // 어두운 파란 뒷면
	const FLinearColor CardBorderColor(0.15f, 0.15f, 0.35f);
	const float BT = 1.5f;

	// AI Player 1 (left side)
	{
		const int32 HandCount = GameState->GetPlayerHand(1).Num();

		// Label
		const FString Label = TEXT("AI 1");
		DrawText(Label, FLinearColor(1.0f, 0.4f, 0.4f), Margin, TopY - 20.0f, HUDFont, 0.8f);

		// Draw card backs
		for (int32 i = 0; i < HandCount; ++i)
		{
			const float CardX = Margin + i * (AICardW + AICardGap);
			const float CardY = TopY;

			// Card back
			DrawRect(CardBackColor, CardX, CardY, AICardW, AICardH);

			// Border
			DrawRect(CardBorderColor, CardX, CardY, AICardW, BT);
			DrawRect(CardBorderColor, CardX, CardY + AICardH - BT, AICardW, BT);
			DrawRect(CardBorderColor, CardX, CardY, BT, AICardH);
			DrawRect(CardBorderColor, CardX + AICardW - BT, CardY, BT, AICardH);
		}
	}

	// AI Player 2 (right side)
	{
		const int32 HandCount = GameState->GetPlayerHand(2).Num();

		// Total width of AI2 cards
		const float TotalW = HandCount * AICardW + (HandCount > 0 ? (HandCount - 1) * AICardGap : 0.0f);
		const float StartX = CachedViewportSize.X - Margin - TotalW;

		// Label
		const FString Label = TEXT("AI 2");
		float LabelW, LabelH;
		GetTextSize(Label, LabelW, LabelH, HUDFont, 0.8f);
		DrawText(Label, FLinearColor(0.4f, 0.6f, 1.0f), CachedViewportSize.X - Margin - LabelW, TopY - 20.0f, HUDFont, 0.8f);

		// Draw card backs
		for (int32 i = 0; i < HandCount; ++i)
		{
			const float CardX = StartX + i * (AICardW + AICardGap);
			const float CardY = TopY;

			// Card back
			DrawRect(CardBackColor, CardX, CardY, AICardW, AICardH);

			// Border
			DrawRect(CardBorderColor, CardX, CardY, AICardW, BT);
			DrawRect(CardBorderColor, CardX, CardY + AICardH - BT, AICardW, BT);
			DrawRect(CardBorderColor, CardX, CardY, BT, AICardH);
			DrawRect(CardBorderColor, CardX + AICardW - BT, CardY, BT, AICardH);
		}
	}
}

void AHRBLexioHUD::DrawTurnInfo()
{
	const float CenterX = CachedViewportSize.X * 0.5f;
	const float TopY = 70.0f;

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

	// Full-screen dark overlay
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f),
		0.0f, 0.0f, CachedViewportSize.X, CachedViewportSize.Y);

	// Winner text
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

	const FLinearColor WinColor = (Winner == HumanPlayerIndex)
		? FLinearColor(1.0f, 0.85f, 0.0f)
		: FLinearColor(1.0f, 0.4f, 0.4f);

	float TextW, TextH;
	GetTextSize(WinnerText, TextW, TextH, HUDFont, 2.5f);
	DrawText(WinnerText, WinColor, CenterX - TextW * 0.5f, CenterY - 60.0f, HUDFont, 2.5f);

	// Remaining cards for each player
	const float InfoStartY = CenterY + 20.0f;
	const float LineHeight = 30.0f;

	const FLinearColor PlayerColors[UHRBLexioGameState::NUM_PLAYERS] = {
		FLinearColor(0.2f, 0.8f, 0.3f),   // You - green
		FLinearColor(1.0f, 0.4f, 0.4f),    // AI 1 - red
		FLinearColor(0.4f, 0.6f, 1.0f)     // AI 2 - blue
	};
	const FString PlayerNames[UHRBLexioGameState::NUM_PLAYERS] = {
		TEXT("You"), TEXT("AI 1"), TEXT("AI 2")
	};

	for (int32 i = 0; i < UHRBLexioGameState::NUM_PLAYERS; ++i)
	{
		const int32 CardsLeft = GameState->GetPlayerHand(i).Num();
		const int32 Score = GameState->GetPlayerScore(i);

		FString InfoText = FString::Printf(TEXT("%s: %d cards remaining | Score: %d"), *PlayerNames[i], CardsLeft, Score);
		FLinearColor InfoColor = (i == Winner) ? FLinearColor(0.3f, 1.0f, 0.3f) : FLinearColor(0.8f, 0.8f, 0.8f);

		float InfoW, InfoH;
		GetTextSize(InfoText, InfoW, InfoH, HUDFont, 1.2f);
		DrawText(InfoText, InfoColor, CenterX - InfoW * 0.5f, InfoStartY + i * LineHeight, HUDFont, 1.2f);
	}

	// Check if all game rounds are complete
	const float BottomInfoY = InfoStartY + UHRBLexioGameState::NUM_PLAYERS * LineHeight + 20.0f;

	if (GameState->IsAllRoundsComplete())
	{
		// FINAL RESULT header
		FString FinalText = TEXT("FINAL RESULT");
		float FTW, FTH;
		GetTextSize(FinalText, FTW, FTH, HUDFont, 1.0f);
		DrawText(FinalText, FLinearColor(1.0f, 0.85f, 0.0f), CenterX - FTW * 0.5f, CenterY - 90.0f, HUDFont, 1.0f);

		// Find overall winner (highest score)
		int32 BestPlayer = 0;
		int32 BestScore = GameState->GetPlayerScore(0);
		for (int32 i = 1; i < UHRBLexioGameState::NUM_PLAYERS; ++i)
		{
			const int32 S = GameState->GetPlayerScore(i);
			if (S > BestScore)
			{
				BestScore = S;
				BestPlayer = i;
			}
		}

		FString OverallWinner = FString::Printf(TEXT("Overall Winner: %s (%d pts)"), *PlayerNames[BestPlayer], BestScore);
		float OWW, OWH;
		GetTextSize(OverallWinner, OWW, OWH, HUDFont, 1.2f);
		DrawText(OverallWinner, PlayerColors[BestPlayer], CenterX - OWW * 0.5f, BottomInfoY, HUDFont, 1.2f);
	}
	else
	{
		// GAME OVER subtitle (round complete, more rounds to go)
		FString GameOverText = TEXT("HAND COMPLETE");
		float GOW, GOH;
		GetTextSize(GameOverText, GOW, GOH, HUDFont, 1.0f);
		DrawText(GameOverText, FLinearColor(0.6f, 0.6f, 0.6f), CenterX - GOW * 0.5f, CenterY - 90.0f, HUDFont, 1.0f);

		FString NextRoundText = TEXT("Round Complete - Next round starting...");
		float NRW, NRH;
		GetTextSize(NextRoundText, NRW, NRH, HUDFont, 1.0f);
		DrawText(NextRoundText, FLinearColor(0.8f, 0.8f, 0.5f), CenterX - NRW * 0.5f, BottomInfoY, HUDFont, 1.0f);
	}
}

void AHRBLexioHUD::DrawRankLegend()
{
	const float StartX = 20.0f;
	const float StartY = CachedViewportSize.Y * 0.45f;
	const float LineHeight = 22.0f;
	const FLinearColor LabelColor(0.7f, 0.7f, 0.5f);

	// Line 1: Number rank
	DrawText(TEXT("Rank: 3 < 4 < 5 < 6 < 7 < 8 < 9 < 1 < 2"), LabelColor, StartX, StartY, HUDFont, 0.9f);

	// Line 2: Suit rank with colored suit names
	const float SuitLineY = StartY + LineHeight;
	float CurX = StartX;

	const FString SuitPrefix = TEXT("Suit: ");
	float PrefW, PrefH;
	GetTextSize(SuitPrefix, PrefW, PrefH, HUDFont, 0.9f);
	DrawText(SuitPrefix, LabelColor, CurX, SuitLineY, HUDFont, 0.9f);
	CurX += PrefW;

	// Cloud
	const FString CloudText = TEXT("Cloud");
	float TmpW, TmpH;
	GetTextSize(CloudText, TmpW, TmpH, HUDFont, 0.9f);
	DrawText(CloudText, GetSuitColor(EHRBCardSuit::Cloud), CurX, SuitLineY, HUDFont, 0.9f);
	CurX += TmpW;

	const FString Sep1 = TEXT(" < ");
	GetTextSize(Sep1, TmpW, TmpH, HUDFont, 0.9f);
	DrawText(Sep1, LabelColor, CurX, SuitLineY, HUDFont, 0.9f);
	CurX += TmpW;

	// Star
	const FString StarText = TEXT("Star");
	GetTextSize(StarText, TmpW, TmpH, HUDFont, 0.9f);
	DrawText(StarText, GetSuitColor(EHRBCardSuit::Star), CurX, SuitLineY, HUDFont, 0.9f);
	CurX += TmpW;

	GetTextSize(Sep1, TmpW, TmpH, HUDFont, 0.9f);
	DrawText(Sep1, LabelColor, CurX, SuitLineY, HUDFont, 0.9f);
	CurX += TmpW;

	// Moon
	const FString MoonText = TEXT("Moon");
	GetTextSize(MoonText, TmpW, TmpH, HUDFont, 0.9f);
	DrawText(MoonText, GetSuitColor(EHRBCardSuit::Moon), CurX, SuitLineY, HUDFont, 0.9f);
	CurX += TmpW;

	GetTextSize(Sep1, TmpW, TmpH, HUDFont, 0.9f);
	DrawText(Sep1, LabelColor, CurX, SuitLineY, HUDFont, 0.9f);
	CurX += TmpW;

	// Sun
	const FString SunText = TEXT("Sun");
	GetTextSize(SunText, TmpW, TmpH, HUDFont, 0.9f);
	DrawText(SunText, GetSuitColor(EHRBCardSuit::Sun), CurX, SuitLineY, HUDFont, 0.9f);

	// Line 3: 5-card combo rank
	const float ComboLineY = SuitLineY + LineHeight;
	DrawText(TEXT("5-Card: Straight < Flush < Full House < 4-Kind < St.Flush"),
		FLinearColor(0.6f, 0.6f, 0.45f, 0.9f), StartX, ComboLineY, HUDFont, 0.9f);
}

void AHRBLexioHUD::DrawRoundInfo()
{
	const float CenterX = CachedViewportSize.X * 0.5f;
	const float TopY = 45.0f;

	const FString RoundText = FString::Printf(TEXT("Hand %d"), GameState->GetRoundNumber());
	float TextW, TextH;
	GetTextSize(RoundText, TextW, TextH, HUDFont, 1.0f);
	DrawText(RoundText, FLinearColor(0.8f, 0.8f, 0.4f), CenterX - TextW * 0.5f, TopY, HUDFont, 1.0f);
}

void AHRBLexioHUD::DrawScoreInfo()
{
	const float CenterX = CachedViewportSize.X * 0.5f;
	const float TopY = 5.0f;

	// Game round info
	const int32 GameRound = GameState->GetGameRoundCount();
	const int32 MaxRounds = UHRBLexioGameState::MAX_GAME_ROUNDS;
	const FString GameRoundText = FString::Printf(TEXT("Game Round: %d/%d"), GameRound, MaxRounds);
	float GRW, GRH;
	GetTextSize(GameRoundText, GRW, GRH, HUDFont, 0.8f);
	DrawText(GameRoundText, FLinearColor(0.6f, 0.7f, 0.8f), CenterX - GRW * 0.5f, TopY, HUDFont, 0.8f);

	// Score display
	const int32 ScoreYou = GameState->GetPlayerScore(0);
	const int32 ScoreAI1 = GameState->GetPlayerScore(1);
	const int32 ScoreAI2 = GameState->GetPlayerScore(2);

	const float ScoreY = TopY + GRH + 2.0f;
	float CurX = CenterX;

	// Pre-calculate total width for centering
	const FString ScoreLabel = TEXT("Score: ");
	const FString YouScore = FString::Printf(TEXT("You: %d"), ScoreYou);
	const FString AI1Score = FString::Printf(TEXT("AI 1: %d"), ScoreAI1);
	const FString AI2Score = FString::Printf(TEXT("AI 2: %d"), ScoreAI2);
	const FString Separator = TEXT(" | ");

	float SLW, SLH, YSW, YSH, A1W, A1H, A2W, A2H, SepW, SepH;
	GetTextSize(ScoreLabel, SLW, SLH, HUDFont, 0.8f);
	GetTextSize(YouScore, YSW, YSH, HUDFont, 0.8f);
	GetTextSize(Separator, SepW, SepH, HUDFont, 0.8f);
	GetTextSize(AI1Score, A1W, A1H, HUDFont, 0.8f);
	GetTextSize(AI2Score, A2W, A2H, HUDFont, 0.8f);

	const float TotalScoreW = SLW + YSW + SepW + A1W + SepW + A2W;
	CurX = CenterX - TotalScoreW * 0.5f;

	DrawText(ScoreLabel, FLinearColor(0.7f, 0.7f, 0.7f), CurX, ScoreY, HUDFont, 0.8f);
	CurX += SLW;

	DrawText(YouScore, FLinearColor(0.2f, 0.8f, 0.3f), CurX, ScoreY, HUDFont, 0.8f);
	CurX += YSW;

	DrawText(Separator, FLinearColor(0.5f, 0.5f, 0.5f), CurX, ScoreY, HUDFont, 0.8f);
	CurX += SepW;

	DrawText(AI1Score, FLinearColor(1.0f, 0.4f, 0.4f), CurX, ScoreY, HUDFont, 0.8f);
	CurX += A1W;

	DrawText(Separator, FLinearColor(0.5f, 0.5f, 0.5f), CurX, ScoreY, HUDFont, 0.8f);
	CurX += SepW;

	DrawText(AI2Score, FLinearColor(0.4f, 0.6f, 1.0f), CurX, ScoreY, HUDFont, 0.8f);
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

FLinearColor AHRBLexioHUD::GetSuitColor(EHRBCardSuit Suit) const
{
	switch (Suit)
	{
	case EHRBCardSuit::Cloud: return FLinearColor(0.5f, 0.55f, 0.65f);
	case EHRBCardSuit::Star:  return FLinearColor(0.85f, 0.75f, 0.2f);
	case EHRBCardSuit::Moon:  return FLinearColor(0.3f, 0.7f, 0.4f);
	case EHRBCardSuit::Sun:   return FLinearColor(0.85f, 0.25f, 0.2f);
	default:                  return FLinearColor::White;
	}
}

FString AHRBLexioHUD::GetSuitSymbol(EHRBCardSuit Suit) const
{
	switch (Suit)
	{
	case EHRBCardSuit::Cloud: return TEXT("C");
	case EHRBCardSuit::Star:  return TEXT("S");
	case EHRBCardSuit::Moon:  return TEXT("M");
	case EHRBCardSuit::Sun:   return TEXT("R");
	default:                  return TEXT("?");
	}
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
