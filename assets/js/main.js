(function () {
  var posts = Array.isArray(window.blogPosts) ? window.blogPosts : [];
  var list = document.querySelector("[data-post-list]");
  var count = document.querySelector("[data-post-count]");

  document.querySelectorAll("[data-year]").forEach(function (node) {
    node.textContent = new Date().getFullYear();
  });

  if (count) {
    count.textContent = posts.length + " notes";
  }

  if (!list) {
    return;
  }

  if (!posts.length) {
    var empty = document.createElement("p");
    empty.className = "empty-state";
    empty.textContent = "아직 정리된 글이 없습니다.";
    list.appendChild(empty);
    return;
  }

  posts.forEach(function (post) {
    var link = document.createElement("a");
    link.className = "post-row";
    link.href = post.href;

    var date = document.createElement("time");
    date.dateTime = post.date;
    date.textContent = post.date;

    var content = document.createElement("span");
    content.className = "post-row-content";

    var titleLine = document.createElement("span");
    titleLine.className = "post-row-title";
    titleLine.textContent = post.title;

    var summary = document.createElement("span");
    summary.className = "post-row-summary";
    summary.textContent = post.summary;

    var meta = document.createElement("span");
    meta.className = "post-row-meta";
    meta.textContent = [post.category].concat(post.tags || []).join(" · ");

    content.appendChild(titleLine);
    content.appendChild(summary);
    content.appendChild(meta);
    link.appendChild(date);
    link.appendChild(content);
    list.appendChild(link);
  });
})();

