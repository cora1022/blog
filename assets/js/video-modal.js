(function () {
  var dialog = document.querySelector("[data-video-dialog]");
  var frame = document.querySelector("[data-video-frame]");
  var title = document.querySelector("#video-dialog-title");
  var closeButton = document.querySelector("[data-video-close]");

  function getEmbedUrl(url) {
    var match = url.match(/youtu\.be\/([^?&]+)/);
    return match ? "https://www.youtube.com/embed/" + match[1] : "";
  }

  function clearFrame() {
    if (frame) {
      frame.innerHTML = "";
    }
  }

  function openDialog(button) {
    if (!dialog || !frame || !title) {
      return;
    }

    var videoUrl = button.getAttribute("data-video-url") || "";
    var videoTitle = button.getAttribute("data-video-title") || "실행 영상";
    title.textContent = videoTitle;
    clearFrame();

    if (videoUrl) {
      var iframe = document.createElement("iframe");
      iframe.src = getEmbedUrl(videoUrl);
      iframe.title = videoTitle;
      iframe.allow = "accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share";
      iframe.allowFullscreen = true;
      frame.appendChild(iframe);
    } else {
      var message = document.createElement("p");
      message.textContent = "영상 링크를 추후 업데이트할 예정입니다.";
      frame.appendChild(message);
    }

    dialog.showModal();
  }

  document.querySelectorAll("[data-video-title]").forEach(function (button) {
    button.addEventListener("click", function () {
      openDialog(button);
    });
  });

  if (closeButton) {
    closeButton.addEventListener("click", function () {
      clearFrame();
      dialog.close();
    });
  }

  if (dialog) {
    dialog.addEventListener("click", function (event) {
      if (event.target === dialog) {
        clearFrame();
        dialog.close();
      }
    });

    dialog.addEventListener("close", clearFrame);
  }
})();
