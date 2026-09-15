// ============================================================
// Secure Web-Based Voting System — Shared Script
// Helper functions used across all pages
// ============================================================

/** Redirects to login page if the current session is not a logged-in voter. */
function guardLogin() {
  if (!VotingData.isVoter()) {
    window.location.href = "login.html";
  }
}

/** Redirects to login page if the current session is not an admin. */
function guardAdmin() {
  if (!VotingData.isAdmin()) {
    window.location.href = "login.html";
  }
}

/** Shows the logout confirmation modal instead of logging out immediately. */
function showLogoutModal(e) {
  e.preventDefault();
  var modal = document.getElementById("logoutModal");
  if (modal) {
    modal.classList.add("active");
  }
}

/** Cancels logout and hides the modal. */
function cancelLogout(e) {
  if (e) e.preventDefault();
  var modal = document.getElementById("logoutModal");
  if (modal) {
    modal.classList.remove("active");
  }
}

/** Confirms logout: clears session and redirects to login page. */
function confirmLogout(e) {
  if (e) e.preventDefault();
  VotingData.logout();
  window.location.href = "login.html";
}

/** Wires up all logout links on the page to use the confirmation modal. */
function setupLogout() {
  document.querySelectorAll("[id^='logoutLink']").forEach(function (link) {
    link.addEventListener("click", showLogoutModal);
  });
  var cancelBtn = document.getElementById("logoutCancelBtn");
  var confirmBtn = document.getElementById("logoutConfirmBtn");
  if (cancelBtn) cancelBtn.addEventListener("click", cancelLogout);
  if (confirmBtn) confirmBtn.addEventListener("click", confirmLogout);
  var modal = document.getElementById("logoutModal");
  if (modal) {
    modal.addEventListener("click", function (e) {
      if (e.target === modal) cancelLogout();
    });
  }
}

/** Formats an ISO date string (YYYY-MM-DD) into a readable format. */
function formatDate(isoDate) {
  const d = new Date(isoDate);
  const months = [
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  ];
  return d.getDate() + " " + months[d.getMonth()] + " " + d.getFullYear();
}

/** Injects the logout confirmation modal HTML into the page. */
function injectLogoutModal() {
  if (document.getElementById("logoutModal")) return;
  var modal = document.createElement("div");
  modal.className = "modal-overlay";
  modal.id = "logoutModal";
  modal.innerHTML =
    '<div class="modal">' +
      '<h3>Are you sure you want to logout?</h3>' +
      '<p>Your session will be ended and you will be redirected to the login page.</p>' +
      '<div class="modal-actions">' +
        '<button class="btn btn-ghost" id="logoutCancelBtn">Cancel</button>' +
        '<button class="btn btn-danger" id="logoutConfirmBtn">Logout</button>' +
      '</div>' +
    '</div>';
  document.body.appendChild(modal);
}
