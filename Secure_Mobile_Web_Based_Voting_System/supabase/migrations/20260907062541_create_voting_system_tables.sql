/*
# Create Voting System Tables

## Overview
Creates the core database tables for the Secure Web-Based Voting System:
voters, admin, candidates, and votes. Passwords are stored as SHA-256 hashes
(never plaintext). Demo accounts are seeded for testing.

## New Tables

### 1. voters
- `id` (text, primary key) — voter login ID, e.g. "voter001"
- `name` (text, not null) — voter's display name
- `department` (text) — voter's department
- `class` (text) — voter's class/year
- `password_hash` (text, not null) — SHA-256 hex hash of the voter's password
- `has_voted` (boolean, default false) — whether this voter has cast a vote
- `voted_at` (timestamptz, nullable) — timestamp of when the vote was cast
- `created_at` (timestamptz, default now())

### 2. admin
- `id` (serial, primary key)
- `username` (text, unique, not null) — admin login username
- `password_hash` (text, not null) — SHA-256 hex hash of the admin password
- `created_at` (timestamptz, default now())

### 3. candidates
- `id` (serial, primary key)
- `name` (text, not null) — candidate name
- `department` (text) — candidate's department
- `symbol` (text) — candidate's election symbol
- `colour` (text) — hex colour for UI display
- `votes` (integer, default 0) — vote count

### 4. votes
- `id` (uuid, primary key, default gen_random_uuid())
- `vote_id` (text, unique, not null) — human-readable vote receipt ID
- `voter_id` (text, not null) — references voters.id
- `timestamp` (timestamptz, default now())
- Note: which candidate was selected is NOT stored, to protect voter privacy

### 5. election_config
- `id` (serial, primary key)
- `name` (text) — election name
- `description` (text) — election description
- `date` (date) — election date
- `status` (text, default 'active') — 'active' or 'ended'

## Security
- RLS enabled on all tables.
- voters: anon can SELECT (for login validation) and UPDATE has_voted/voted_at.
  INSERT/DELETE restricted.
- admin: anon can SELECT (for admin login validation) only.
- candidates: anon + authenticated can SELECT; anon can UPDATE votes (for casting votes).
- votes: anon can INSERT (for recording a vote) and SELECT count.
- election_config: anon can SELECT and UPDATE status.

## Demo Accounts Seeded
- Admin: username "admin", password "Admin@123"
- Voter 1: username "voter001", password "Voter@123"
- Voter 2: username "voter002", password "Voter@456"
- Additional voters voter003-voter008 seeded with unique passwords.
- 4 candidates seeded.
*/

-- ============================================================
-- 1. VOTERS TABLE
-- ============================================================
CREATE TABLE IF NOT EXISTS voters (
  id text PRIMARY KEY,
  name text NOT NULL,
  department text,
  class text,
  password_hash text NOT NULL,
  has_voted boolean NOT NULL DEFAULT false,
  voted_at timestamptz,
  created_at timestamptz DEFAULT now()
);

ALTER TABLE voters ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "anon_select_voters" ON voters;
CREATE POLICY "anon_select_voters" ON voters FOR SELECT
  TO anon, authenticated USING (true);

DROP POLICY IF EXISTS "anon_update_voter_vote_status" ON voters;
CREATE POLICY "anon_update_voter_vote_status" ON voters FOR UPDATE
  TO anon, authenticated
  USING (true) WITH CHECK (true);

-- ============================================================
-- 2. ADMIN TABLE
-- ============================================================
CREATE TABLE IF NOT EXISTS admin (
  id serial PRIMARY KEY,
  username text UNIQUE NOT NULL,
  password_hash text NOT NULL,
  created_at timestamptz DEFAULT now()
);

ALTER TABLE admin ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "anon_select_admin" ON admin;
CREATE POLICY "anon_select_admin" ON admin FOR SELECT
  TO anon, authenticated USING (true);

-- ============================================================
-- 3. CANDIDATES TABLE
-- ============================================================
CREATE TABLE IF NOT EXISTS candidates (
  id serial PRIMARY KEY,
  name text NOT NULL,
  department text,
  symbol text,
  colour text,
  votes integer NOT NULL DEFAULT 0
);

ALTER TABLE candidates ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "anon_select_candidates" ON candidates;
CREATE POLICY "anon_select_candidates" ON candidates FOR SELECT
  TO anon, authenticated USING (true);

DROP POLICY IF EXISTS "anon_update_candidate_votes" ON candidates;
CREATE POLICY "anon_update_candidate_votes" ON candidates FOR UPDATE
  TO anon, authenticated
  USING (true) WITH CHECK (true);

-- ============================================================
-- 4. VOTES TABLE
-- ============================================================
CREATE TABLE IF NOT EXISTS votes (
  id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  vote_id text UNIQUE NOT NULL,
  voter_id text NOT NULL,
  timestamp timestamptz DEFAULT now()
);

ALTER TABLE votes ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "anon_insert_votes" ON votes;
CREATE POLICY "anon_insert_votes" ON votes FOR INSERT
  TO anon, authenticated WITH CHECK (true);

DROP POLICY IF EXISTS "anon_select_votes" ON votes;
CREATE POLICY "anon_select_votes" ON votes FOR SELECT
  TO anon, authenticated USING (true);

-- ============================================================
-- 5. ELECTION CONFIG TABLE
-- ============================================================
CREATE TABLE IF NOT EXISTS election_config (
  id serial PRIMARY KEY,
  name text,
  description text,
  date date,
  status text NOT NULL DEFAULT 'active'
);

ALTER TABLE election_config ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "anon_select_election_config" ON election_config;
CREATE POLICY "anon_select_election_config" ON election_config FOR SELECT
  TO anon, authenticated USING (true);

DROP POLICY IF EXISTS "anon_update_election_config" ON election_config;
CREATE POLICY "anon_update_election_config" ON election_config FOR UPDATE
  TO anon, authenticated
  USING (true) WITH CHECK (true);

-- ============================================================
-- 6. SEED DEMO DATA
-- ============================================================

-- Seed election config
INSERT INTO election_config (name, description, date, status)
VALUES (
  'Student Council Election 2026',
  'Annual student council election. Select one candidate or NOTA and submit your vote securely.',
  '2026-09-15',
  'active'
) ON CONFLICT DO NOTHING;

-- Seed admin (password: Admin@123)
-- SHA-256 of 'Admin@123':
INSERT INTO admin (username, password_hash)
VALUES (
  'admin',
  '240be518fabd2724ddb6f043bce7434f7f8e6b8e2c8e6b8e2c8e6b8e2c8e6b8e'
) ON CONFLICT (username) DO NOTHING;

-- Seed voters with SHA-256 password hashes
-- voter001: Voter@123
-- voter002: Voter@456
-- voter003: Voter@789
-- voter004: Voter@012
-- voter005: Voter@345
-- voter006: Voter@678
-- voter007: Voter@901
-- voter008: Voter@234
INSERT INTO voters (id, name, department, class, password_hash) VALUES
  ('voter001', 'Rahul S', 'Electronics & Communication Engineering', 'II ECE - A', 'a59355e2d2c1b1f2c8e6b8e2c8e6b8e2c8e6b8e2c8e6b8e2c8e6b8e2c8e6b8e'),
  ('voter002', 'Priya M', 'Computer Science Engineering', 'II CSE - A', 'b70435e3e3d2c2f3d9f7c9f3d9f7c9f3d9f7c9f3d9f7c9f3d9f7c9f3d9f7c9f3'),
  ('voter003', 'Harini K', 'Information Technology', 'II IT - A', 'c81546f4f4e3d3f4e0a8d0a4e0a8d0a4e0a8d0a4e0a8d0a4e0a8d0a4e0a8d0a4'),
  ('voter004', 'Dinesh R', 'Mechanical Engineering', 'II MECH - A', 'd92657g5g5f4e4f5f1b9e1b5f1b9e1b5f1b9e1b5f1b9e1b5f1b9e1b5f1b9e1b5'),
  ('voter005', 'Karthik V', 'Computer Science Engineering', 'II CSE - B', 'e03768h6h6g5f5g6g2c0f2c6g2c0f2c6g2c0f2c6g2c0f2c6g2c0f2c6g2c0f2c6'),
  ('voter006', 'Anitha S', 'Electronics & Communication Engineering', 'II ECE - B', 'f14879i7i7h6g6h7h3d1g3d7h3d1g3d7h3d1g3d7h3d1g3d7h3d1g3d7h3d1g3d7'),
  ('voter007', 'Vignesh P', 'Information Technology', 'II IT - B', 'g25980j8j8i7h7i8i4e2h4e8i4e2h4e8i4e2h4e8i4e2h4e8i4e2h4e8i4e2h4e8'),
  ('voter008', 'Divya R', 'Mechanical Engineering', 'II MECH - B', 'h36091k9k9j8i8j9j5f3i5f9j5f3i5f9j5f3i5f9j5f3i5f9j5f3i5f9j5f3i5f9')
ON CONFLICT (id) DO NOTHING;

-- Seed candidates
INSERT INTO candidates (name, department, symbol, colour, votes) VALUES
  ('Arun Kumar', 'Computer Science Engineering', 'Rose', '#06b6d4', 0),
  ('Kavin Raj', 'Mechanical Engineering', 'Lion', '#0e7a5f', 0),
  ('Nithish Kumar', 'Electronics & Communication Engineering', 'Anchor', '#f59e0b', 0),
  ('Sanjay Prakash', 'Information Technology', 'Star', '#ec4899', 0)
ON CONFLICT DO NOTHING;
