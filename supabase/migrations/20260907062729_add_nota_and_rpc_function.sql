/*
# Add NOTA votes column and vote casting RPC function

## Overview
1. Adds a `nota_votes` integer column to `election_config` for tracking NOTA votes.
2. Creates a `cast_vote` SECURITY DEFINER function that atomically:
   - Checks if voter has already voted
   - Increments candidate votes (or NOTA votes)
   - Inserts a vote record with a unique vote ID
   - Updates the voter's has_voted status
   - Returns the vote receipt ID

## Modified Tables
- `election_config`: added `nota_votes` integer column (default 0)

## New Functions
- `cast_vote(p_voter_id text, p_selection text)`:
  - p_voter_id: the voter's ID (e.g. 'voter001')
  - p_selection: candidate ID as text (e.g. '1') or 'NOTA'
  - Returns: { vote_id text, success boolean, message text }

## Security
- Function is SECURITY DEFINER so it can update all tables atomically.
- Callable by anon and authenticated roles.
- Input parameters are validated inside the function.
*/

-- Add nota_votes column to election_config
ALTER TABLE election_config ADD COLUMN IF NOT EXISTS nota_votes integer NOT NULL DEFAULT 0;

-- Create the cast_vote RPC function
CREATE OR REPLACE FUNCTION cast_vote(p_voter_id text, p_selection text)
RETURNS json
LANGUAGE plpgsql
SECURITY DEFINER
SET search_path = public
AS $$
DECLARE
  v_voter RECORD;
  v_vote_id text;
  v_candidate_id int;
  v_result json;
BEGIN
  -- Check if voter exists and hasn't voted
  SELECT * INTO v_voter FROM voters WHERE id = p_voter_id;
  IF NOT FOUND THEN
    RETURN json_build_object('success', false, 'message', 'Voter not found.');
  END IF;

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
    -- Check candidate exists
    IF NOT EXISTS (SELECT 1 FROM candidates WHERE id = v_candidate_id) THEN
      RETURN json_build_object('success', false, 'message', 'Invalid candidate selection.');
    END IF;
    -- Increment candidate votes atomically
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

-- Grant execute permission to anon and authenticated
GRANT EXECUTE ON FUNCTION cast_vote(text, text) TO anon, authenticated;
