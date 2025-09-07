-- Enable UUID function used by Prisma on Postgres/Neon
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- Create table
CREATE TABLE IF NOT EXISTS "Session" (
  "id" uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  "terminalId" text NOT NULL,
  "amount" integer NOT NULL,
  "currency" text NOT NULL,
  "status" text NOT NULL DEFAULT 'pending',
  "createdAt" timestamptz NOT NULL DEFAULT now(),
  "updatedAt" timestamptz NOT NULL DEFAULT now()
);

-- Keep updatedAt fresh
CREATE OR REPLACE FUNCTION set_updated_at() RETURNS trigger AS $$
BEGIN NEW."updatedAt" = now(); RETURN NEW; END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_session_updated_at ON "Session";
CREATE TRIGGER trg_session_updated_at
BEFORE UPDATE ON "Session"
FOR EACH ROW EXECUTE FUNCTION set_updated_at();
