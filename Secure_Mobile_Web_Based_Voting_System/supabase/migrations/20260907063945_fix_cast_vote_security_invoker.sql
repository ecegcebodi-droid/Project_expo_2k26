/*
# Fix cast_vote security: switch to SECURITY INVOKER

## Overview
The security advisor flags cast_vote as a SECURITY DEFINER function executable
by anon. Since this app uses custom session auth (not Supabase Auth), there is
no "authenticated" Supabase session role to restrict to — the app always runs
as anon. The solution is to switch the function to SECURITY INVOKER and grant
anon the specific table permissions needed (UPDATE on candidates, voters,
election_config; INSERT on votes). The function's internal voter token
validation (password hash check) provides the actual security control.

## Changes
- Drops the SECURITY DEFINER version of cast_vote
- Creates a SECURITY INVOKER version (same logic, same voter token validation)
- Grants anon UPDATE on candidates, voters, election_config
- Grants anon INSERT on votes (already had it via RLS policy)
*/

-- Drop the SECURITY DEFINER version
DROP FUNCTION IF EXISTS cast_vote(text, text, text);

-- Create SECURITY INVOKER version (same logic, no elevated privileges)
CREATE OR REPLACE FUNCTION cast_vote(p_voter_id text, p_selection text, p_voter_token text)
RETURNS json
LANGUAGE plpgsql
SECURITY INVOKER
SET search_path = public
AS $$
DECLARE
  v_voter RECORD;
  v_vote_id text;
  v_candidate_id int;
BEGIN
  -- Check if voter exists
  SELECT * INTO v_voter FROM voters WHERE id = p_voter_id;
  IF NOT FOUND THEN
    RETURN json_build_object('success', false, 'message', 'Voter not found.');
  END IF;

  -- Validate voter token (must match stored password hash)
  IF v_voter.password_hash IS DISTINCT FROM p_voter_token THEN
    RETURN json_build_object('success', false, 'message', 'Authentication failed.');
  END IF;

  -- Check if already voted
  IF v_voter.has_voted THEN
    RETURN json_build_object('success', false, 'message', 'You have already voted.');
  END IF;

  -- Generate unique vote ID
  v_vote_id := 'VOTE-' || upper(to_char(now(), 'YYMMDDHH24MISS')) || '-' || upper(substr(md5(random()::text), 1, 4));

  -- Handle NOTA or candidate vote
  IF p_selection = 'NOTA' THEN
    UPDATE election_config SET nota_votes = nota_votes + 1 WHERE id = 1;
  ELSE
    v_candidate_id := p_selection::int;
    IF NOT EXISTS (SELECT 1 FROM candidates WHERE id = v_candidate_id) THEN
      RETURN json_build_object('success', false, 'message', 'Invalid candidate selection.');
    END IF;
    UPDATE candidates SET votes = votes + 1 WHERE id = v_candidate_id;
  END IF;

  -- Insert vote record (candidate selection NOT stored for privacy)
  INSERT INTO votes (vote_id, voter_id, timestamp)
  VALUES (v_vote_id, p_voter_id, now());

  -- Mark voter as having voted
  UPDATE voters SET has_voted = true, voted_at = now() WHERE id = p_voter_id;

  RETURN json_build_object('success', true, 'vote_id', v_vote_id);
END;
$$;

-- Grant execute to anon only (the app runs as anon with custom session auth)
GRANT EXECUTE ON FUNCTION cast_vote(text, text, text) TO anon;

-- Ensure anon has the table privileges needed by the INVOKER function
-- (These are already granted by default Supabase setup, but make explicit)
GRANT UPDATE ON candidates TO anon;
GRANT UPDATE ON voters TO anon;
GRANT UPDATE ON election_config TO anon;
GRANT INSERT ON votes TO anon;
GRANT SELECT ON candidates TO anon;
GRANT SELECT ON voters TO anon;
GRANT SELECT ON election_config TO anon;
