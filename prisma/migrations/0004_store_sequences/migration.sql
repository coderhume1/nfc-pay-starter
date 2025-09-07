
CREATE TABLE IF NOT EXISTS "TerminalSequence" (
  "storeCode" text PRIMARY KEY,
  "last" integer NOT NULL DEFAULT 0,
  "updatedAt" timestamptz NOT NULL DEFAULT now()
);
