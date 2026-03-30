---
name: Confluence API 접근 방식
description: curl로 Confluence API를 직접 호출하면 403이 반환됨. MCP 도구(conf_get 등)가 필요하나 현재 QA 에이전트 컨텍스트에서는 MCP가 로드되지 않을 수 있음
type: project
---

curl로 krafton.atlassian.net에 직접 API 호출 시 403 (`Current user not permitted to use Confluence`) 반환.

**Why:** Confluence MCP 서버(`@aashari/mcp-server-atlassian-confluence`)가 별도로 실행되어야 하며, claude.json에 설정된 토큰을 통해야만 접근 가능한 것으로 추정.

**How to apply:** QA 리뷰 시 위키 기획서를 읽어야 할 때, curl 시도 없이 바로 confluence Skill을 로드하고 MCP 도구를 사용해야 한다. MCP 도구가 없으면 요청서에 명시된 검증 항목으로 대체 진행.
