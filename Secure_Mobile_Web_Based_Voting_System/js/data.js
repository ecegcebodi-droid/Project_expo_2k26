// ============================================================
// Secure Web-Based Voting System — Data Module (Supabase)
// Uses Supabase REST API for all data operations.
// Passwords are hashed with SHA-256 in the browser before
// being compared against the database hash.
// ============================================================

// ---------- Supabase config (public anon key, safe for frontend) ----------
var _SUPABASE_URL = "https://elnjxetdmvfhohwiafvd.supabase.co";
var _SUPABASE_ANON_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImVsbmp4ZXRkbXZmaG9od2lhZnZkIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODg3NjA3MDQsImV4cCI6MjEwNDMzNjcwNH0.qdcI04_qGsq4cZb1lCdOBEL_o_iBf6moeUmttan-d6Q";

// ---------- SHA-256 hash function (async, uses Web Crypto API) ----------
async function sha256(message) {
  var msgBuffer = new TextEncoder().encode(message);
  var hashBuffer = await crypto.subtle.digest("SHA-256", msgBuffer);
  var hashArray = Array.from(new Uint8Array(hashBuffer));
  return hashArray.map(function (b) { return b.toString(16).padStart(2, "0"); }).join("");
}

// ---------- Supabase REST API helpers ----------
function supabaseHeaders() {
  return {
    "apikey": _SUPABASE_ANON_KEY,
    "Authorization": "Bearer " + _SUPABASE_ANON_KEY,
    "Content-Type": "application/json",
  };
}

async function supabaseSelect(table, columns, filters) {
  columns = columns || "*";
  filters = filters || {};
  var url = _SUPABASE_URL + "/rest/v1/" + table + "?select=" + encodeURIComponent(columns);
  var keys = Object.keys(filters);
  for (var i = 0; i < keys.length; i++) {
    url += "&" + keys[i] + "=eq." + encodeURIComponent(filters[keys[i]]);
  }
  var res = await fetch(url, { headers: supabaseHeaders() });
  if (!res.ok) throw new Error("Database query failed: " + table);
  return await res.json();
}

async function supabaseUpdate(table, data, filters) {
  var url = _SUPABASE_URL + "/rest/v1/" + table + "?";
  var keys = Object.keys(filters);
  for (var i = 0; i < keys.length; i++) {
    url += keys[i] + "=eq." + encodeURIComponent(filters[keys[i]]) + "&";
  }
  url = url.slice(0, -1);
  var headers = supabaseHeaders();
  headers["Prefer"] = "return=minimal";
  var res = await fetch(url, {
    method: "PATCH",
    headers: headers,
    body: JSON.stringify(data),
  });
  if (!res.ok) throw new Error("Database update failed: " + table);
}

async function supabaseRpc(fn, params) {
  var url = _SUPABASE_URL + "/rest/v1/rpc/" + fn;
  var res = await fetch(url, {
    method: "POST",
    headers: supabaseHeaders(),
    body: JSON.stringify(params),
  });
  if (!res.ok) throw new Error("RPC call failed: " + fn);
  return await res.json();
}

// ============================================================
// VotingData — Main data controller
// ============================================================

var VotingData = {
  _electionCache: null,
  _candidatesCache: null,
  _votersCache: null,

  // ---------- Election info ----------
  getElection: async function () {
    if (this._electionCache) return this._electionCache;
    var rows = await supabaseSelect("election_config", "*");
    this._electionCache = rows && rows.length > 0 ? rows[0] : null;
    return this._electionCache;
  },

  // ---------- Candidates ----------
  getCandidates: async function () {
    if (this._candidatesCache) return this._candidatesCache;
    var rows = await supabaseSelect("candidates", "*");
    this._candidatesCache = rows;
    return rows;
  },

  // ---------- Voters (public list for display) ----------
  getVoters: async function () {
    if (this._votersCache) return this._votersCache;
    var rows = await supabaseSelect("voters", "id,username,name,department,class,has_voted,voted_at");
    this._votersCache = rows;
    return rows;
  },

  // ---------- Voter login (by username) ----------
  login: async function (username, password) {
    var hash = await sha256(password);
    var rows = await supabaseSelect("voters", "id,username,name,department,class,has_voted,password_hash", { username: username });
    if (!rows || rows.length === 0) return null;
    var voter = rows[0];
    if (voter.password_hash !== hash) return null;
    sessionStorage.setItem("voterId", voter.id);
    sessionStorage.setItem("voterUsername", voter.username);
    sessionStorage.setItem("voterName", voter.name);
    sessionStorage.setItem("voterDepartment", voter.department);
    sessionStorage.setItem("voterClass", voter.class);
    sessionStorage.setItem("hasVoted", voter.has_voted ? "true" : "false");
    sessionStorage.setItem("role", "voter");
    sessionStorage.setItem("voterToken", hash);
    sessionStorage.removeItem("isAdmin");
    return voter;
  },

  // ---------- Admin login ----------
  adminLogin: async function (username, password) {
    var hash = await sha256(password);
    var rows = await supabaseSelect("admin", "username,password_hash", { username: username });
    if (!rows || rows.length === 0) return false;
    if (rows[0].password_hash !== hash) return false;
    sessionStorage.setItem("isAdmin", "true");
    sessionStorage.setItem("role", "admin");
    sessionStorage.removeItem("voterId");
    sessionStorage.removeItem("voterName");
    sessionStorage.removeItem("hasVoted");
    sessionStorage.removeItem("voteId");
    sessionStorage.removeItem("voterToken");
    return true;
  },

  isAdmin: function () {
    return sessionStorage.getItem("isAdmin") === "true";
  },

  logout: function () {
    sessionStorage.removeItem("voterId");
    sessionStorage.removeItem("voterUsername");
    sessionStorage.removeItem("voterName");
    sessionStorage.removeItem("voterDepartment");
    sessionStorage.removeItem("voterClass");
    sessionStorage.removeItem("hasVoted");
    sessionStorage.removeItem("voteId");
    sessionStorage.removeItem("isAdmin");
    sessionStorage.removeItem("role");
    sessionStorage.removeItem("voterToken");
  },

  isLoggedIn: function () {
    return sessionStorage.getItem("voterId") !== null || this.isAdmin();
  },

  isVoter: function () {
    return sessionStorage.getItem("voterId") !== null && !this.isAdmin();
  },

  getCurrentVoter: function () {
    return sessionStorage.getItem("voterId");
  },

  getCurrentVoterName: function () {
    return sessionStorage.getItem("voterName");
  },

  getCurrentVoterDepartment: function () {
    return sessionStorage.getItem("voterDepartment");
  },

  getCurrentVoterClass: function () {
    return sessionStorage.getItem("voterClass");
  },

  hasVoted: function () {
    return sessionStorage.getItem("hasVoted") === "true";
  },

  // ---------- Cast vote (atomic via RPC) ----------
  castVote: async function (selection) {
    var voterId = this.getCurrentVoter();
    if (!voterId) return { success: false, message: "Not logged in." };
    if (this.hasVoted()) return { success: false, message: "You have already voted." };

    try {
      var voterToken = sessionStorage.getItem("voterToken");
      var result = await supabaseRpc("cast_vote", {
        p_voter_id: voterId,
        p_selection: String(selection),
        p_voter_token: voterToken,
      });
      if (result && result.success) {
        sessionStorage.setItem("hasVoted", "true");
        sessionStorage.setItem("voteId", result.vote_id);
        this._candidatesCache = null;
        this._votersCache = null;
        this._electionCache = null;
        return { success: true, voteId: result.vote_id };
      }
      return { success: false, message: (result && result.message) || "Vote failed." };
    } catch (err) {
      return { success: false, message: "Network error. Please try again." };
    }
  },

  getVoteId: function () {
    return sessionStorage.getItem("voteId");
  },

  // ---------- Results ----------
  getTotalVotes: async function () {
    var rows = await supabaseSelect("votes", "id");
    return rows ? rows.length : 0;
  },

  getResults: async function () {
    var candidates = await this.getCandidates();
    var election = await this.getElection();
    var totalVotes = await this.getTotalVotes();
    var notaVotes = election ? (election.nota_votes || 0) : 0;
    var total = totalVotes;

    var candidateResults = candidates.map(function (c) {
      return {
        id: c.id,
        name: c.name,
        department: c.department,
        symbol: c.symbol,
        colour: c.colour,
        votes: c.votes,
        percentage: total > 0 ? ((c.votes / total) * 100).toFixed(1) : "0.0",
      };
    });
    var notaResult = {
      id: "NOTA",
      name: "NOTA",
      label: "None of the Above",
      colour: "#64748b",
      votes: notaVotes,
      percentage: total > 0 ? ((notaVotes / total) * 100).toFixed(1) : "0.0",
    };
    return { candidates: candidateResults, nota: notaResult };
  },

  getWinner: async function () {
    var results = await this.getResults();
    var all = results.candidates.concat([results.nota]);
    var maxVotes = Math.max.apply(null, all.map(function (c) { return c.votes; }));
    if (maxVotes === 0) return null;
    var winners = all.filter(function (c) { return c.votes === maxVotes; });
    if (winners.length > 1) return { tie: true, names: winners.map(function (w) { return w.name; }) };
    return { tie: false, name: winners[0].name, votes: winners[0].votes };
  },

  // ---------- Admin stats ----------
  getStats: async function () {
    var voters = await this.getVoters();
    var totalVotes = await this.getTotalVotes();
    var election = await this.getElection();
    var totalVoters = voters.length;
    var votedCount = voters.filter(function (v) { return v.has_voted; }).length;
    var notVotedCount = voters.filter(function (v) { return !v.has_voted; }).length;
    var notaVotes = election ? (election.nota_votes || 0) : 0;
    var votingPercentage = totalVoters > 0
      ? ((totalVotes / totalVoters) * 100).toFixed(1)
      : "0.0";
    return {
      totalVoters: totalVoters,
      totalVotes: totalVotes,
      votedCount: votedCount,
      notVotedCount: notVotedCount,
      notaVotes: notaVotes,
      votingPercentage: votingPercentage,
      electionStatus: election ? election.status : "active",
    };
  },

  // ---------- Voter lists for admin ----------
  getVotedVoters: async function () {
    var voters = await this.getVoters();
    return voters
      .filter(function (v) { return v.has_voted; })
      .map(function (v) {
        return {
          id: v.id,
          name: v.name,
          department: v.department,
          class: v.class,
          status: "Voted",
          votedAt: v.voted_at,
        };
      });
  },

  getNotVotedVoters: async function () {
    var voters = await this.getVoters();
    return voters
      .filter(function (v) { return !v.has_voted; })
      .map(function (v) {
        return {
          id: v.id,
          name: v.name,
          department: v.department,
          class: v.class,
          status: "Not Voted",
        };
      });
  },

  getDepartments: async function () {
    var voters = await this.getVoters();
    var seen = {};
    var result = [];
    for (var i = 0; i < voters.length; i++) {
      if (!seen[voters[i].department]) {
        seen[voters[i].department] = true;
        result.push(voters[i].department);
      }
    }
    return result;
  },

  getClasses: async function () {
    var voters = await this.getVoters();
    var seen = {};
    var result = [];
    for (var i = 0; i < voters.length; i++) {
      if (!seen[voters[i].class]) {
        seen[voters[i].class] = true;
        result.push(voters[i].class);
      }
    }
    return result;
  },

  // ---------- Election status toggle (admin) ----------
  setElectionStatus: async function (status) {
    await supabaseUpdate("election_config", { status: status }, { id: 1 });
    this._electionCache = null;
  },

  // ---------- Reset demo election (admin only) ----------
  resetElection: async function () {
    try {
      var result = await supabaseRpc("reset_election", {});
      if (result && result.success) {
        this._candidatesCache = null;
        this._votersCache = null;
        this._electionCache = null;
        return { success: true, message: result.message };
      }
      return { success: false, message: (result && result.message) || "Reset failed." };
    } catch (err) {
      return { success: false, message: "Network error. Please try again." };
    }
  },
};
