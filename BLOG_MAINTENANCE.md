# Blog Maintenance Guide

이 문서는 `cora1022.github.io/blog` 블로그를 다음 작업에서도 이어서 관리하기 위한 메모입니다.

## 저장소 정보

- 로컬 경로: `C:\Users\70sou\Desktop\github\blog`
- GitHub 저장소: `https://github.com/cora1022/blog`
- 배포 주소: `https://cora1022.github.io/blog/`
- 배포 브랜치: `main`
- Pages 설정: `main` 브랜치의 `/` 루트

## 현재 구조

```text
blog/
├─ index.html
├─ 404.html
├─ README.md
├─ BLOG_MAINTENANCE.md
├─ assets/
│  ├─ css/
│  │  └─ blog.css
│  ├─ js/
│  │  ├─ main.js
│  │  ├─ posts.js
│  │  └─ video-modal.js
│  └─ code/
│     └─ *.cpp
└─ posts/
   └─ nypc-participation-timeline.html
```

## 디자인 방향

- 종이 질감의 독립 블로그로 유지한다.
- 폰트는 Google Fonts의 `Gowun Batang`, `Gowun Dodum`을 사용한다.
- 포트폴리오 메인으로 이어지는 링크는 넣지 않는다.
- 상단 오른쪽에는 GitHub 아이콘과 `cora1022`만 둔다.
- 글 본문은 `paper-panel`, `article`, `article-content` 구조를 따른다.

## 글 목록 관리

글 목록은 `assets/js/posts.js`에서 관리한다.

새 글을 추가할 때:

1. `posts/example.html` 파일을 만든다.
2. `assets/js/posts.js`의 `window.blogPosts` 배열에 항목을 추가한다.
3. 최신 글이 위에 오도록 배열 순서를 정리한다.

예시:

```js
{
  title: "글 제목",
  date: "2026-07-10",
  category: "개발기록",
  href: "posts/example.html",
  summary: "목록에 표시될 짧은 설명입니다.",
  tags: ["태그1", "태그2"]
}
```

## 현재 NYPC 글

- 파일: `posts/nypc-participation-timeline.html`
- 제목: `NYPC 참가 타임라인`
- 목록 등록: `assets/js/posts.js`
- 코드 다운로드 파일: `assets/code/*.cpp`
- 영상 모달 스크립트: `assets/js/video-modal.js`

## 제출 코드 관리

제출 코드는 `assets/code/{제출번호}.cpp`에 둔다.

현재 등록된 코드:

```text
32620.cpp
35107.cpp
40655.cpp
47806.cpp
49903.cpp
58894.cpp
62539.cpp
67492.cpp
77166.cpp
80882.cpp
94157.cpp
104693.cpp
```

주의:

- 블로그에 올리는 코드는 주석을 제거한 버전으로 유지한다.
- 원본 파일은 `Downloads`에 있을 수 있지만, 배포되는 파일은 `assets/code/` 안의 사본이다.
- 코드 버튼은 CSS에서 `</>` 아이콘으로 보이도록 처리한다.

## 영상 링크 관리

각 제출 카드의 YouTube 버튼은 `posts/nypc-participation-timeline.html` 안의 `data-video-url`로 관리한다.

예시:

```html
<button
  class="submission-action youtube-action"
  type="button"
  data-video-title="#32620 실행 영상"
  data-video-url="https://youtu.be/u1oa255PkIc"
  aria-label="#32620 실행 영상 보기">
</button>
```

아직 링크가 없는 제출은 `data-video-url`을 비워둔다. 그러면 모달에는 `영상 링크를 추후 업데이트할 예정입니다.` 문구가 표시된다.

현재 링크 대기 상태:

- `#94157`
- `#104693`

## 검증 명령

변경 후 아래 명령을 실행한다.

```powershell
node --check assets\js\main.js
node --check assets\js\posts.js
node --check assets\js\video-modal.js
git diff --check
git status --short
```

## 배포 절차

```powershell
git add -A
git commit -m "Update blog"
git push origin main
gh api repos/cora1022/blog/pages --jq '{html_url, status, source}'
```

Pages 상태가 `building`이면 잠시 기다린 뒤 다시 확인한다.

```powershell
Start-Sleep -Seconds 8
gh api repos/cora1022/blog/pages --jq '{html_url, status, source}'
```

## 최근 미배포 변경 메모

다음 변경이 로컬에 있을 수 있다.

- NYPC 글 상단에 마스터트랙과 NEXT NATION 룰 설명 서론 추가
- 설명 영상 링크 추가: `https://www.youtube.com/watch?v=BnExruRtP1A&t=114s`
- `About Creative` 메타 문구 제거

