---
name: HUD Canvas 직접 접근 패턴 경고
description: DrawHUD() 외부에서 Canvas->SizeX/Y를 직접 사용하면 null 크래시 발생 가능. CachedViewportSize 패턴 사용 권장
type: feedback
---

`Canvas`는 `DrawHUD()` 호출 중에만 유효하다. DrawHUD()에서 호출되는 하위 함수들도 직접 `Canvas->SizeX/Y`를 참조하면, 향후 리팩토링 시 DrawHUD 외부로 옮겨질 경우 크래시 위험이 있다.

**Why:** `HRBLexioHUD`에서 Canvas null 크래시 수정을 위해 `CachedViewportSize`를 도입했는데, 일부 하위 함수들이 여전히 `Canvas->SizeX/Y`를 직접 참조하고 있어 일관성 문제가 발견됨 (Step 2 QA).

**How to apply:** QA 시 HUD 코드에서 `Canvas->SizeX` 또는 `Canvas->SizeY` 패턴을 검색하고, `DrawHUD()` 캐시 업데이트 1줄 외에는 모두 `CachedViewportSize.X/Y`로 교체한다.
