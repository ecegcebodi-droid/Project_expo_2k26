/*
# Update to 20 demo voters with new credentials, new admin password, reset function

## Changes
1. Add `username` column to voters table
2. Delete all existing voters, insert 20 new voters (V001-V020)
3. Update admin password to admin123 (SHA-256)
4. Reset candidates to 4 with correct names/departments/symbols
5. Reset all vote counts, NOTA, votes table
6. Add reset_election RPC function for admin demo reset
*/

-- Add username column to voters
ALTER TABLE voters ADD COLUMN IF NOT EXISTS username text;

-- Delete all existing voters
DELETE FROM voters;

-- Insert 20 new voters with SHA-256 password hashes
-- Passwords: pass001 through pass020
INSERT INTO voters (id, username, name, department, class, password_hash, has_voted) VALUES
  ('V001', 'voter01', 'Arun Kumar', 'Computer Science Engineering', 'II CSE - A', 'e495c8e1d6723467c5c840b71744533eae94e9ca4b68823178e8a2df150f1121', false),
  ('V002', 'voter02', 'Priya S', 'Computer Science Engineering', 'II CSE - B', '9c0cdeb1dad83648e3674bc4835a65a6e398ae4b5bee7c72cd3e68e5b7ccbe47', false),
  ('V003', 'voter03', 'Harini K', 'Electronics and Communication Engineering', 'II ECE - A', '31e0377d72256fa00da681f58bdc01778989cc7855f159cdc03841d3e37760d2', false),
  ('V004', 'voter04', 'Dinesh R', 'Mechanical Engineering', 'II MECH - A', '0a8f421b8c7c448ac9591487670488491b963573d4292e72dc96d477752a3665', false),
  ('V005', 'voter05', 'Karthik V', 'Information Technology', 'II IT - A', '42fc98b7d2c8537ace34d207a389fd149be0809913f1fd90c8d356eb392a3ccb', false),
  ('V006', 'voter06', 'Anitha S', 'Computer Science Engineering', 'II CSE - A', '70dbe3bf25b7e5442988012e0b2d31c919baa208b73bf9d4a3b4c4489f46bd01', false),
  ('V007', 'voter07', 'Vignesh P', 'Electronics and Communication Engineering', 'II ECE - B', '4885c3e2969f513158d8a161bf95be6d293df67e695fab9ec5a906ed144517d7', false),
  ('V008', 'voter08', 'Divya R', 'Mechanical Engineering', 'II MECH - B', '40bd682b462ca6dcebd6f45ae04dcc287fedf10fdd4e3558f7aa842f8249373a', false),
  ('V009', 'voter09', 'Surya N', 'Information Technology', 'II IT - B', 'd8cb9ab2dc74fd7c592909084a364b6fd2243427633f06456f6317237d374c88', false),
  ('V010', 'voter10', 'Meena K', 'Computer Science Engineering', 'II CSE - B', 'd8e935eb1eafecd0d66463861b067dfc6e098cca210bcc615a0b71d0939bd662', false),
  ('V011', 'voter11', 'Rahul M', 'Electronics and Communication Engineering', 'II ECE - A', 'e107c6c1dc4110d5a24e5f0e6fe0308f1156b0aa26837a80f3221997a7f187e5', false),
  ('V012', 'voter12', 'Sneha R', 'Mechanical Engineering', 'II MECH - A', '1c6b4f2278067b875faa584b186908eb8c93289fbcb1ae3b057f66fc3c48853a', false),
  ('V013', 'voter13', 'Gokul S', 'Information Technology', 'II IT - A', 'c4784f5f6210bd6e79d2193d482e84ddc28d5e5cd4903e99ec2fd83bddef9c8b', false),
  ('V014', 'voter14', 'Lakshmi N', 'Computer Science Engineering', 'II CSE - A', 'b2dc644fb6abbbf84d02da443d96f396c685a0f2592ffdbef6fc85dbf1badebf', false),
  ('V015', 'voter15', 'Praveen K', 'Electronics and Communication Engineering', 'II ECE - B', 'cc4718e76e5206265e5fbddeac7dcff655d41f7cce76bfa8f0ee3677660e78da', false),
  ('V016', 'voter16', 'Deepika S', 'Mechanical Engineering', 'II MECH - B', '382fce0d8c6f2c543a91c404bb02aae42985d45ed403fe5f66a96aa495c2e430', false),
  ('V017', 'voter17', 'Mohan R', 'Information Technology', 'II IT - B', '9a491c9773070fa989989c6edb750d6f57778a9eb141033a6184061b40e013f6', false),
  ('V018', 'voter18', 'Ishwarya M', 'Computer Science Engineering', 'II CSE - B', '596ff0823118e6f4d3ba426be993d1d43ee183ece0d58eafa66823715a442838', false),
  ('V019', 'voter19', 'Sakthi V', 'Electronics and Communication Engineering', 'II ECE - A', '04fb848e5b61444dc8f46be426827cbc7a28fcf4b87a675926fad5f81b368296', false),
  ('V020', 'voter20', 'Naveen K', 'Mechanical Engineering', 'II MECH - A', 'c12eece46500c9bd02f1cc061a0043929d3a84db864df831852750cde9f5288e', false);

-- Update admin password to admin123
UPDATE admin SET password_hash = '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9' WHERE username = 'admin';

-- Delete and re-insert candidates with correct names
DELETE FROM candidates;
INSERT INTO candidates (name, department, symbol, colour, votes) VALUES
  ('Arun Raj', 'Computer Science Engineering', 'Rose', '#06b6d4', 0),
  ('Kavin Kumar', 'Mechanical Engineering', 'Lion', '#0e7a5f', 0),
  ('Nithish Kumar', 'Electronics and Communication Engineering', 'Anchor', '#f59e0b', 0),
  ('Sanjay Prakash', 'Information Technology', 'Star', '#ec4899', 0);

-- Clear all votes
DELETE FROM votes;

-- Reset NOTA count and election config
UPDATE election_config SET nota_votes = 0, status = 'active' WHERE id = 1;

-- Create reset_election RPC function
CREATE OR REPLACE FUNCTION reset_election()
RETURNS json
LANGUAGE plpgsql
SECURITY INVOKER
SET search_path = public
AS $$
BEGIN
  -- Reset all voters to not voted
  UPDATE voters SET has_voted = false, voted_at = null;
  -- Reset candidate votes
  UPDATE candidates SET votes = 0;
  -- Reset NOTA
  UPDATE election_config SET nota_votes = 0;
  -- Clear vote records
  DELETE FROM votes;
  RETURN json_build_object('success', true, 'message', 'Election reset successfully.');
END;
$$;

GRANT EXECUTE ON FUNCTION reset_election() TO anon;
