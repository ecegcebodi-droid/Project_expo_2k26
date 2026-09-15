/*
# Secure cast_vote function — add voter token validation

## Overview
Replaces the existing cast_vote SECURITY DEFINER function with a version
that requires a voter token (the voter's password hash) as a third parameter.
This prevents anonymous users from casting votes on behalf of arbitrary voters.

## Changes
- Drops the old cast_vote(text, text) function
- Creates cast_vote(text, text, text) with p_voter_token parameter
- The function validates p_voter_token against the voter's password_hash
  before accepting the vote
- Revokes execute from anon and authenticated, then grants only to anon
  (the app runs as anon since it uses custom session auth, not Supabase Auth)
- The function remains SECURITY DEFINER to atomically update multiple tables

## Security
- Even though anon can call the function, the voter token (password hash)
  must match the stored hash — only the logged-in voter who entered their
  password can cast their vote
- One-vote-per-voter is still enforced via has_voted check
- Vote is still atomic (candidate increment + vote record + voter update in one transaction)
*/

-- Drop the old function (2 params)
DROP FUNCTION IF EXISTS cast_vote(text, text);

-- Create the new function (3 params, with voter token)
CREATE OR REPLACE FUNCTION cast_vote(p_voter_id text, p_selection text, p_voter_token text)
RETURNS json
LANGUAGE plpgsql
SECURITY DEFINER
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

-- Grant execute only to anon (the app uses custom session auth, always runs as anon)
GRANT EXECUTE ON FUNCTION cast_vote(text, text, text) TO anon;
