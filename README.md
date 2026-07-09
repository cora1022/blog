# cora1022 Blog

`https://cora1022.github.io/blog/`에서 제공할 정적 블로그입니다.

## 구조

- `index.html`: 글 목록 홈
- `assets/css/blog.css`: 종이 질감 블로그 스타일
- `assets/js/posts.js`: 글 목록 메타데이터
- `assets/js/main.js`: 글 목록 렌더링
- `posts/*.html`: 개별 글 HTML

## 새 글 추가 방법

1. `posts/new-post.html` 형식으로 글 HTML을 만든다.
2. `assets/js/posts.js`의 `window.blogPosts` 배열에 글 정보를 추가한다.
3. 최신 글이 위에 오도록 배열 순서를 정리한다.

