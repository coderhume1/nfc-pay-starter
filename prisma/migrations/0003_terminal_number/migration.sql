
CREATE TABLE IF NOT EXISTS "TerminalNumber" (
  "id" serial PRIMARY KEY,
  "createdAt" timestamptz NOT NULL DEFAULT now()
);
