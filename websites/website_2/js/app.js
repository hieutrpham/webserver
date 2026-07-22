(function () {
  "use strict";

  function updateStatus() {
    var target = document.getElementById("js-status");
    if (!target) return;
    var now = new Date();
    target.classList.add("success");
    target.innerHTML = "<strong>JavaScript loaded successfully.</strong><br>Local time: " +
      now.toLocaleString() + "<br>Viewport: " + window.innerWidth + " × " + window.innerHeight;
  }

  function wireDeleteHelper() {
    var form = document.getElementById("delete-form");
    var output = document.getElementById("delete-result");
    if (!form || !output) return;
    form.addEventListener("submit", function (event) {
      event.preventDefault();
      var path = document.getElementById("delete-path").value.trim();
      if (!path) return;
      fetch(path, { method: "DELETE" })
        .then(function (response) {
          output.textContent = "DELETE " + path + " returned HTTP " + response.status;
        })
        .catch(function (error) {
          output.textContent = "Request failed: " + error.message;
        });
    });
  }

  updateStatus();
  wireDeleteHelper();
}());
