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

			const FString NumStr = FString::Printf(TEXT("%d"), Entry.Combination.Cards[c].Number);
			float TextW, TextH;
			GetTextSize(NumStr, TextW, TextH, HUDFont, 1.5f);
			const FLinearColor TextCol = FLinearColor::Black;
			DrawText(NumStr, TextCol,
				CardX + (HistoryCardW - TextW) * 0.5f,
				DrawY + (HistoryCardH - TextH) * 0.5f,
				HUDFont, 1.5f);
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

	for (int32 i = 0; i < UHRBLexioGameState::NUM_PLAYERS; ++i)
	{
		const int32 CardsLeft = GameState->GetPlayerHand(i).Num();
		FString PlayerName;
		if (i == HumanPlayerIndex)
		{
			PlayerName = TEXT("You");
		}
		else
		{
			PlayerName = FString::Printf(TEXT("AI %d"), i);
		}

		FString InfoText = FString::Printf(TEXT("%s: %d cards remaining"), *PlayerName, CardsLeft);
		FLinearColor InfoColor = (i == Winner) ? FLinearColor(0.3f, 1.0f, 0.3f) : FLinearColor(0.8f, 0.8f, 0.8f);

		float InfoW, InfoH;
		GetTextSize(InfoText, InfoW, InfoH, HUDFont, 1.2f);
		DrawText(InfoText, InfoColor, CenterX - InfoW * 0.5f, InfoStartY + i * LineHeight, HUDFont, 1.2f);
	}

	// "GAME OVER" subtitle
	FString GameOverText = TEXT("GAME OVER");
	float GOW, GOH;
	GetTextSize(GameOverText, GOW, GOH, HUDFont, 1.0f);
	DrawText(GameOverText, FLinearColor(0.6f, 0.6f, 0.6f), CenterX - GOW * 0.5f, CenterY - 90.0f, HUDFont, 1.0f);
}

void AHRBLexioHUD::DrawRankLegend()
{
	// 화면 좌측 중간에 카드 모양으로 서열 표시
	const float StartX = 20.0f;
	const float StartY = CachedViewportSize.Y * 0.45f;
	const float LegendCardW = 50.0f;
	const float LegendCardH = 68.0f;
	const float LegendGap = 8.0f;
	const float BT = 1.5f;

	// "Rank" 라벨
	DrawText(TEXT("Rank (weak -> strong)"), FLinearColor(0.7f, 0.7f, 0.5f), StartX, StartY - 30.0f, HUDFont, 1.0f);

	// 족보 종류 표시 (카드 아래)
	const float ComboY = StartY + LegendCardH + 15.0f;
	DrawText(TEXT("Single: 1 card  |  Pair: same x2  |  Triple: same x3"),
		FLinearColor(0.6f, 0.6f, 0.45f, 0.9f), StartX, ComboY, HUDFont, 0.9f);

	// 서열 순서: 3,4,5,6,7,8,9,1,2
	const int32 RankOrder[] = { 3, 4, 5, 6, 7, 8, 9, 1, 2 };

	for (int32 i = 0; i < 9; ++i)
	{
		const float CardX = StartX + i * (LegendCardW + LegendGap);
		const float CardY = StartY;

		// 카드 배경 (약한 쪽은 어둡고 강한 쪽은 밝게)
		const float Brightness = 0.6f + 0.4f * (static_cast<float>(i) / 8.0f);
		const FLinearColor CardBg(Brightness * 0.9f, Brightness * 0.95f, Brightness, 0.9f);
		DrawRect(CardBg, CardX, CardY, LegendCardW, LegendCardH);

		// 테두리
		const FLinearColor BorderColor(0.3f, 0.35f, 0.3f);
		DrawRect(BorderColor, CardX, CardY, LegendCardW, BT);
		DrawRect(BorderColor, CardX, CardY + LegendCardH - BT, LegendCardW, BT);
		DrawRect(BorderColor, CardX, CardY, BT, LegendCardH);
		DrawRect(BorderColor, CardX + LegendCardW - BT, CardY, BT, LegendCardH);

		// 숫자
		const FString NumStr = FString::Printf(TEXT("%d"), RankOrder[i]);
		float TextW, TextH;
		GetTextSize(NumStr, TextW, TextH, HUDFont, 1.2f);
		DrawText(NumStr, FLinearColor::Black,
			CardX + (LegendCardW - TextW) * 0.5f,
			CardY + (LegendCardH - TextH) * 0.5f,
			HUDFont, 1.2f);

		// 화살표 (마지막 카드 뒤에는 안 그림)
		if (i < 8)
		{
			const float ArrowX = CardX + LegendCardW + 1.0f;
			DrawText(TEXT("<"), FLinearColor(0.6f, 0.6f, 0.4f),
				ArrowX, CardY + LegendCardH * 0.5f - 10.0f, HUDFont, 0.9f);
		}
	}
}

void AHRBLexioHUD::DrawRoundInfo()
{
	const float CenterX = CachedViewportSize.X * 0.5f;
	const float TopY = 30.0f;

	const FString RoundText = FString::Printf(TEXT("Round %d"), GameState->GetRoundNumber());
	float TextW, TextH;
	GetTextSize(RoundText, TextW, TextH, HUDFont, 1.0f);
	DrawText(RoundText, FLinearColor(0.8f, 0.8f, 0.4f), CenterX - TextW * 0.5f, TopY, HUDFont, 1.0f);
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
